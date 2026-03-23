;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-12.

;;; Eval following region

(define root (new-sogroup))
(define pendulum (new-sopendulum))
(-> root 'addchild pendulum)
(-> root 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;; To get initial movement.
(-> (-> pendulum 'rotation0) 'setvalue (new-sbvec3f 0 0 1) 1.57)
(-> (-> pendulum 'rotation1) 'setvalue (new-sbvec3f 0 0 1) 3.14)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoPendulum node, play around with these ;;;;;;;;;;;

(-> (-> pendulum 'speed) 'setvalue 0.5)
(-> (-> pendulum 'rotation0) 'setvalue (new-sbvec3f 1 0 1) 0.00)
(-> (-> pendulum 'rotation1) 'setvalue (new-sbvec3f 1 0 1) 3.14)

;; turn off then back on
(-> (-> pendulum 'on) 'setvalue 0)
(-> (-> pendulum 'on) 'setvalue 1)

;; from parent class SoRotation
(-> (-> pendulum 'rotation) 'setvalue (new-sbvec3f 1 1 1) 0.00)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Confirmed and potential bugs. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; None


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; False alarms (ex-possible bugs) and ex-bugs ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; None


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoPendulum ;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> root 'copy 1))

;; Export scenegraph with SoPendulum.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with SoPendulum in it.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { Pendulum { rotation0 0 1 0 0.00  rotation1 0 1 1 1.57 }  Cube {} }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(-> viewer 'viewall)
(-> viewer 'setscenegraph (new-socube))
