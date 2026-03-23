;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-10-03.

(define basekit (new-sobasekit))

;; Get SoNodeKitListPart (SoBaseKit's "callbackList" part)
(define nodekitlist (sonodekitlistpart-cast (-> basekit 'getpart (new-sbname "callbackList") 1)))
(-> nodekitlist 'addchild (new-soeventcallback))

(define writeaction (new-sowriteaction))
(-> writeaction 'apply nodekitlist)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

