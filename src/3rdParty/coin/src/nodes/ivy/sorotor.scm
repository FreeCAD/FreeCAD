;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-11.

;;; Eval following region

(define root (new-sogroup))
(define rotor (new-sorotor))
(-> root 'addchild rotor)
(-> root 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoRotor node, play around with these ;;;;;;;;;;;;;;

(-> (-> rotor 'speed) 'setvalue 0.2)

;; turn off then back on
(-> (-> rotor 'on) 'setvalue 0)
(-> (-> rotor 'on) 'setvalue 1)

;; from parent class SoRotation
(-> (-> rotor 'rotation) 'setvalue (new-sbvec3f 1 0 0) 1.57)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoRotor ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> root 'copy 1))

;; Export scenegraph with SoRotor.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with SoRotor in it.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { Rotor { speed 0.2  on TRUE }  Cube {} }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(-> viewer 'viewall)
(-> viewer 'setscenegraph (new-socube))
