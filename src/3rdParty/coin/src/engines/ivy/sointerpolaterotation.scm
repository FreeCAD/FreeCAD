;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-27.

;;; Eval following region

;; Make scene graph and first viewer
(define text (new-sotext3))

(define interpolaterotation (new-sointerpolaterotation))
(-> (-> text 'string) 'connectFrom (-> interpolaterotation 'output))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph text)
(-> viewer 'show)

;;; End initial eval-region

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoInterpolateRotation engine, play around ;;;;;;;;;;;;

(-> (-> interpolaterotation 'input0) 'setvalue (new-sbvec3f 0 1 0) 1.57)
(-> (-> interpolaterotation 'input1) 'setvalue (new-sbvec3f 1 0 0) 1.57)

(-> (-> interpolaterotation 'alpha) 'setvalue 0.5)


;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))
(-> viewer-copy 'show)

;; Export scenegraph with engine.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with engine in it.
(let ((buffer "#Inventor V2.1 ascii\n\n Text3 { string \"\" = InterpolateRotation { alpha 0.99  input0 [ 0 1 0 1.57 ]  input1 [ 0 0 1 1.57 ] } . output }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Confirmed and potential bugs. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Fixed bugs and false alarms (ex-possible bugs) ;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> text 'justification) 'setValue SoText3::CENTER)
(-> (-> text 'parts) 'setValue SoText3::ALL)
(-> (-> text 'string) 'disconnect)
(-> viewer 'viewAll)
