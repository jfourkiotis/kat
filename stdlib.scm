(define number? integer?)

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (length items)
  (define (iter a count)
    (if (null? a)
      count
      (iter (cdr a) (+ 1 count))))
  (iter items 0))

(write 'length)

(define (append list1 list2)
  (if (null? list1)
    list2
    (cons (car (list1) (append (cdr list1) list2)))))

(write 'append)

(define (reverse l)
  (define (iter in out)
    (if (pair? in)
      (iter (cdr in) (cons (car in) out))
      out))
  (iter l '()))

(write 'reverse)

(define (map proc items)
  (if (null? items)
    '()
    (cons (proc (car items))
          (map proc (cdr items)))))

(write 'map)

(define (for-each f l)
  (if (null? l)
    #t
    (begin
      (f (car l))
      (for-each f (cdr l)))))

(write 'for-each)

(define (not x)
  (if x #f #t))

(write 'not)

'stlib-loaded

