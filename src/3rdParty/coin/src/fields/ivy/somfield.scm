;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-15.

;;; Set up connection SoText3::string<-SoColorIndex::index
;;; (i.e. SoMFString<-SoMFInt32).

(define text (new-sotext3))
(-> text 'ref)
(define colorindex (new-socolorindex))
(-> colorindex 'ref)
(-> (-> text 'string) 'connectFrom (-> colorindex 'index))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test SoM*::setNum() ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(set-mfield-values! (-> colorindex 'index) 0 '(0 1 2 3 5 7 11))
;; Make smaller
(-> (-> colorindex 'index) 'setnum 5)
;; Make larger
(-> (-> colorindex 'index) 'setnum 9)

;; text->string.getNum() should equal booloperation->a.getNum() at all
;; times. (Note that SGI/TGS Open Inventor doesn't propagate
;; "shrinkage", i.e. setNum() calls where the new value is less than
;; the old won't influence number of values on slave fields -- which
;; is probably a bug.)
(-> (-> colorindex 'index) 'getnum)
(-> (-> text 'string) 'getnum)

;; Get fields printed out.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply text)
(-> writeaction 'apply colorindex)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; FIXME: mucho testing of the SoMField should be inserted here. The
;; few bits above were just written ad hoc while actually debugging
;; other parts of the library. 20000915 mortene.



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> text 'justification) 'setValue SoText3::CENTER)
(-> (-> text 'string) 'disconnect)
(-> viewer 'viewAll)
