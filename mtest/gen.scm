(begin

(define %export-to-scheme '())

; generate a .h and .c file, write to these files
(define %h (open-output-file "s7gen.h" "w"))
(define %c (open-output-file "s7gen.cpp" "w"))

(define (emit-header-getset objname memname)
  (format %h "s7_pointer ms_~a_~a (s7_scheme *sc, s7_pointer args);~%" objname memname)
  (format %h "s7_pointer ms_set_~a_~a (s7_scheme *sc, s7_pointer args);~%" objname memname))

; write the beginning of a c-function. FIX: (with-c-fun (args) body) would be better
(define (emit-func-epilog setter objname memname objtype)
  (format %c "
s7_pointer ms_~a~a_~a (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    ~a* o = (~a*) g->cd;
" (if setter "set_" "") objname memname objtype objtype))

(define (emit-goo-setters objtype objname memname memnameset type)
  `(let ()
     (set! %export-to-scheme (cons (list ,objname ,memname)
                                   %export-to-scheme))
     (emit-header-getset ,objname ,memname)
     (emit-func-epilog #f ,objname ,memname ,objtype)
     (format %c "
  return s7_make_~a(sc, o->~a());
}
" ',type ,memname)
     (emit-func-epilog #t ,objname ,memname ,objtype)
     (format %c "
    s7_pointer x = s7_cadr(args);
    o->~a(s7_~a(x));
    return x;
}
" ,memnameset ',type)))

(define-macro (def-goo-setters objtype objname memname memnameset type)
  ; if no setter-name is provided, generate it from getter (camelCase)
  (let ((memnameset (or memnameset
                        (format #f "set~a~a"
                                (string-upcase (substring memname 0 1))
                                (substring memname 1)))))
    (case type
      ((integer)
       (emit-goo-setters objtype objname memname memnameset type))
      ((real)
       (emit-goo-setters objtype objname memname memnameset type))
      (else
       (error "UNKNOWN goo-get/set type: " type)))))
#|
(define-macro (def-goo-setters-real objtype objname memname memnameset)
    `(let ()
       (set! %export-to-scheme (cons (list ,objname ,memname)
                                     %export-to-scheme))
       (emit-header-getset ,objname ,memname)
       (emit-func-epilog #f ,objname ,memname ,objtype)
       (format %c "
  return s7_make_real(sc, o->~a());
}
" ,memname)
       (emit-func-epilog #t ,objname ,memname ,objtype)
       (format %c "
    s7_pointer x = s7_cadr(args);
    o->~a(s7_real(x));
    return x;
}
" ,memnameset))))

|#
(define-macro (def-goo-setters-bool objtype objname memname memnameget memnameset)
  `(let ()
     (set! %export-to-scheme (cons (list ,objname ,memname)
                                   %export-to-scheme))
     (emit-header-getset ,objname ,memname)
     (emit-func-epilog #f ,objname ,memname ,objtype)
     (format %c "
    return s7_make_boolean(sc, o->~a());
}
" ,memnameget)
     (emit-func-epilog #t ,objname ,memname ,objtype)
     (format %c "
  s7_pointer x = s7_cadr(args);
  o->~a(s7_boolean(sc, x));
  return x;
}
" ,memnameset)))

(define-macro (def-goo-setters-sym objtype objname memname memnameset transname)
  `(let ()
     (set! %export-to-scheme (cons (list ,objname ,memname)
                                   %export-to-scheme))
     (emit-header-getset ,objname ,memname)
     (emit-func-epilog #f ,objname ,memname ,objtype)
     (format %c "
    return s7_make_symbol(sc, ~a_to_string(o->~a()));
}
" ,transname ,memname)
     (emit-func-epilog #t ,objname ,memname ,objtype)
     (format %c "
  s7_pointer x = s7_cadr(args);
  o->~a(string_to_~a(s7_symbol_name(x)));
  return x;
}
" ,memnameset ,transname)))

(define (emit-c-type-string-maps name typelst basetypename)
  (let ()
    (format %h "const char* ~a_to_string (~a x);~%" name basetypename)
    (format %c "const char* ~a_to_string (~a x)~%" name basetypename)
    (format %c "{~%    switch (x) {~%")
    (map (lambda (lst)
                (match lst
                  ((symname c-type)
                   (format %c "        case ~a:~%" c-type)
                   (format %c "        return \"~s\";~%" symname))))
         typelst)
    (format %c "    }~%}~%")

    (format %h "~a string_to_~a (const char *name);~%" basetypename name)
    (format %c "~a string_to_~a (const char *name)~%" basetypename name)
    (format %c "{~%~%")
    (map (lambda (lst)
                (match lst
                  ((symname c-type)
                   (format %c "    if (!strcmp(name, \"~s\")) {~%" symname)
                   (format %c "        return ~a;~%" c-type)
                   (format %c "    }~%"))))
         typelst)
    (format %c "    return (~a)0;~%" basetypename)
    (format %c "}~%")))

(define (emit-c-type-string-maps-simple name typelst basetypename)
  (let ()
    ; emit c-function that takes a object of a specific type and returns a string
    (format %h "const char* ~a_to_string (~a x);~%" name basetypename)
    (format %c "const char* ~a_to_string (~a x)~%" name basetypename)
    (format %c "{~%    switch (x) {~%")
    (for-each (lambda (symname)
      (format %c "        case ~a::~a:~%" basetypename symname)
      (format %c "        return \"~a\";~%" symname))
      typelst)
    (format %c "    }~%}~%")
    ; emit c-function that takes an string and returns an object of a specific type
    (format %h "~a string_to_~a (const char *name);~%" basetypename name)
    (format %c "~a string_to_~a (const char *name)~%" basetypename name)
    (format %c "{~%~%")
    (for-each (lambda (symname)
      (format %c "    if (!strcmp(name, \"~a\")) {~%" symname)
      (format %c "        return ~a::~a;~%" basetypename symname)
      (format %c "    }~%"))
      typelst)
    (format %c "    return (~a)0;~%" basetypename)
    (format %c "}~%")))

(define (gen-done) 
  (format #t "closing output port~%")
  (close-output-port %h)
  (close-output-port %c))

)
