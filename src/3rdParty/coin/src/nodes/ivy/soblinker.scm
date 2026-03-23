;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-07.

;;; Eval following region

(define blinker (new-soblinker))
(-> blinker 'addchild (new-socone))
(-> blinker 'addchild (new-sosphere))
(-> blinker 'addchild (new-socylinder))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph blinker)
(-> viewer 'show)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoBlinker ndde, play around with these ;;;;;;;;;;;;

(-> (-> blinker 'speed) 'setValue 0.3)
(-> (-> blinker 'whichChild) 'setValue 1)
(-> blinker 'addchild (new-socube))
(-> blinker 'removechild 0)

(begin ; turn off then back on
  (-> (-> blinker 'on) 'setValue 0)
  (-> (-> blinker 'on) 'setValue 1)
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoBlinker ;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> blinker 'copy 1))
(-> viewer-copy 'show)

;; Test copy.
(define blinkercopy (soblinker-cast (-> viewer-copy 'getscenegraph)))
(-> (-> blinkercopy 'on) 'setvalue 1)

;; Export scenegraph with SoBlinker.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Export a copied blinker node.
(let* ((blinker (new-soblinker))
       (writeaction (new-sowriteaction))
       (blinker-copy (-> blinker 'copy 1)))
  (-> writeaction 'apply blinker-copy))

;; Read scenegraph with SoBlinker in it.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { Blinker { Material { diffuseColor 1 0 0 } Material { diffuseColor 1 1 0 } }  Cube {} }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> viewer 'viewAll)
(-> viewer 'setscenegraph (new-socube))

(-> (-> blinker 'whichchild) 'getvalue)
(-> (-> blinkercopy 'whichchild) 'getvalue)

(-> (-> blinker 'on) 'setvalue 0)

(display (-> (soblinker-cast (-> viewer 'getscenegraph)) 'getnumchildren))

(begin
  (-> (-> blinker 'on) 'setvalue 1)
  (-> (-> blinkercopy 'on) 'setvalue 1))
