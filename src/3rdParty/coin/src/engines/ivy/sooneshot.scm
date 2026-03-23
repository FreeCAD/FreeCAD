;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-27.

;;; Eval following region

;; Make scene graph and first viewer
(define oneshot (new-sooneshot))
(define root (new-soseparator))

(begin
  ;; timeOut SoText3
  (let ((timeout-text (new-sotext3)))
    (-> (-> timeout-text 'string) 'connectFrom (-> oneshot 'timeout))
    (-> root 'addchild timeout-text))
  ;; SoTranslation
  (let ((translation (new-sotranslation)))
    (-> (-> translation 'translation) 'setvalue (new-sbvec3f 0 -10 0))
    (-> root 'addchild translation))
  ;; ramp SoText3
  (let ((ramp-text (new-sotext3)))
    (-> (-> ramp-text 'string) 'connectFrom (-> oneshot 'ramp))
    (-> root 'addchild ramp-text))
  ;; SoTranslation
  (let ((translation (new-sotranslation)))
    (-> (-> translation 'translation) 'setvalue (new-sbvec3f 0 -10 0))
    (-> root 'addchild translation))
  ;; isActive SoText3
  (let ((isactive-text (new-sotext3)))
    (-> (-> isactive-text 'string) 'connectFrom (-> oneshot 'isactive))
    (-> root 'addchild isactive-text)))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoOneShot engine, play around ;;;;;;;;;;;;;;;;;;;;;

(-> (-> oneshot 'duration) 'setvalue (new-sbtime 3.0))
(-> (-> oneshot 'trigger) 'setvalue)
(-> (-> oneshot 'disable) 'setvalue 0)
(-> (-> oneshot 'flags) 'setvalue SoOneShot::HOLD_FINAL)
(-> (-> oneshot 'flags) 'setvalue SoOneShot::RETRIGGERABLE)

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))
(-> viewer-copy 'show)

;; Export scenegraph with engine.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> viewer 'viewall)
