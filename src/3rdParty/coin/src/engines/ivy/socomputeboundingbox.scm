;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-22.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-22.

;;; Eval following region

;; Read scenegraph with engine in it.
(define root
  (let ((buffer "#Inventor V2.1 ascii\n\n\
Separator { \
  Separator { \
    DEF the_cone Cone { bottomRadius 2.0  height 6.0 } \
  } \
  DEF spheresep Separator { \
    DEF spheretrans Transform { translation 5 5 0  scaleFactor 0.8 8.0 1.5 } \
    DEF the_sphere Sphere { } \
  } \
  Separator { \
    Translation { translation 10 0 0 } \
    DEF bboxtext SoText3 { string \"hepp\" } \
  } \
}")
        (input (new-soinput)))
    (-> input 'setbuffer (void-cast buffer) (string-length buffer))
    (sodb::readall input)))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region

;; Pick out nodes we'll be working with.
(define spheresep (sobase::getnamedbase (new-sbname "spheresep")
                                        (soseparator::getclasstypeid)))
(define spheretrans
  (sotransform-cast (sobase::getnamedbase (new-sbname "spheretrans")
                                          (sotransform::getclasstypeid))))
(define sphere
  (sosphere-cast (sobase::getnamedbase (new-sbname "the_sphere")
                                       (sosphere::getclasstypeid))))
(define cone
  (socone-cast (sobase::getnamedbase (new-sbname "the_cone")
                                     (socone::getclasstypeid))))
(define text (sotext3-cast (sobase::getnamedbase (new-sbname "bboxtext")
                                                 (sotext3::getclasstypeid))))

;; Make and connect SoComputeBoundingBox engine to the Sphere node.
(define computebbox (new-socomputeboundingbox))
(-> (-> computebbox 'node) 'setvalue (sonode-cast spheresep))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoComputeBoundingBox engine, play around ;;;;;;;;;;

;; Check values of all outputs.
(-> (-> text 'string) 'connectfrom (-> computebbox 'min))
(-> (-> text 'string) 'connectfrom (-> computebbox 'max))
(-> (-> text 'string) 'connectfrom (-> computebbox 'boxcenter))
(-> (-> text 'string) 'connectfrom (-> computebbox 'objectcenter))

;; Modify the sphere to see that notification works.
(-> (-> sphere 'radius) 'setvalue 1)
;; Modify the sphere transform to see that notification works.
(-> (-> spheretrans 'scalefactor) 'setvalue 1 1 1)

;; Find path to cone.
(define conepath
  (let ((searchaction (new-sosearchaction)))
    (-> searchaction 'setname (new-sbname "the_cone"))
    (-> searchaction 'apply root)
    (-> searchaction 'getpath)))

;; Connect SoComputeBoundingBox engine to path.
(-> (-> computebbox 'path) 'setvalue conepath)

;; Change cone to see that notification works.
(-> (-> cone 'height) 'setvalue 50)


;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))
(-> viewer-copy 'show)

;; Export scenegraph with engine.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with engine in it.
(let ((buffer "#Inventor V2.1 ascii\n\n Separator { DEF the_sphere Sphere { }  Translation { translation 0 10 0 }  Text3 { string \"\" = ComputeBoundingBox { node USE the_sphere } . max } }")
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
