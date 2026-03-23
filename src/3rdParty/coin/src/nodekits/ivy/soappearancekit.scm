;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-10-03.

;;; Eval following region

(define root (new-sogroup))
(define appearancekit (new-soappearancekit))
(-> root 'addchild appearancekit)
(-> root 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region

;; Pick out the material node, and change its diffuseColor.
(define material (somaterial-cast (-> appearancekit 'getpart
                                      (new-sbname "material") 1)))
(-> (-> material 'diffusecolor) 'setvalue (new-sbvec3f 0.8 0.4 0.4))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the scenegraph ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))

;; Export scenegraph with kit.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with kit.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { AppearanceKit { material Material { diffuseColor 0.9 0.9 0.0 } }  Sphere { } }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
