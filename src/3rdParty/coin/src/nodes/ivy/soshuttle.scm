;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-12.

;;; Eval following region

(define root (new-sogroup))
(define shuttle (new-soshuttle))
(-> root 'addchild shuttle)
(-> root 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;; To get initial movement (default is from <0, 0, 0> to <0, 0, 0>).
(-> (-> shuttle 'translation0) 'setvalue 0 1 0)
(-> (-> shuttle 'translation1) 'setvalue 0 -1 0)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoShuttle node, play around with these ;;;;;;;;;;;;

(-> (-> shuttle 'speed) 'setvalue 0.5)
(-> (-> shuttle 'translation0) 'setvalue 1 1 0)
(-> (-> shuttle 'translation1) 'setvalue -1 1 0)

;; turn off then back on
(-> (-> shuttle 'on) 'setvalue 0)
(-> (-> shuttle 'on) 'setvalue 1)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Confirmed and potential bugs. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; None


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; False alarms (ex-possible bugs) and ex-bugs ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Setting translation field manually used to freeze the animation,
;; see Bugzilla #197.  The reason for this was an unsafe optimization in
;; SoEngine::notify().

;; from parent class SoTranslation
(-> (-> shuttle 'translation) 'setvalue '#(1 0 0))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoShuttle ;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> root 'copy 1))

;; Export scenegraph with SoShuttle.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with SoShuttle in it.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { Shuttle { translation0 -1 0 0  translation1 1 0 0 }  Sphere {} }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(-> viewer 'viewall)
(-> viewer 'setscenegraph (new-socube))
