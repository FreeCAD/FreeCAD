;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-10-03.

;;; Eval following region

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph (new-socone))
(-> viewer 'show)

;;; End initial eval-region

(-> viewer 'setglrenderaction (new-soglrenderaction (new-sbviewportregion)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Export kit.
(define basekit (new-sobasekit))
(define writeaction (new-sowriteaction))
(define eventcblist (sonodekitlistpart-cast (-> basekit 'getpart (new-sbname "callbackList") 1)))
(-> eventcblist 'addchild (new-soeventcallback))

(-> writeaction 'apply eventcblist)

(-> basekit 'ref)
; Write
(-> writeaction 'apply basekit)
(-> basekit 'unref)

(let ((basekit (new-sobasekit))
      (writeaction (new-sowriteaction))
      (eventcbnode (-> basekit 'getpart (new-sbname "callbackList") 1)))
  (-> basekit 'ref)
  (display eventcbnode)
  ; Write
  (-> writeaction 'apply basekit)
  (-> basekit 'unref))
