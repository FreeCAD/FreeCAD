;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-07.

;;; Eval following region

;; Make scene graph and first viewer
(define text (new-sotext3))

(define timecounter (new-sotimecounter))
(-> (-> text 'string) 'connectFrom (-> timecounter 'output))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph text)
(-> viewer 'setdecoration 0)
(-> viewer 'setsize (new-sbvec2s 128 128))
(-> viewer 'show)

;;; End initial eval-region

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoTimeCounter engine, play around ;;;;;;;;;;;;;;;;;

(-> (-> timecounter 'min) 'setValue 0)
(-> (-> timecounter 'max) 'setValue 10)
(-> (-> timecounter 'frequency) 'setValue 0.4)
(-> (-> timecounter 'step) 'setValue 1)
(-> (-> timecounter 'syncIn) 'setValue) ; trigger restart at min value

;; This is for testing the syncOut engine output.

(define (syncout-callback user-data sensor)
  (format #t "You called, Master?~%"))

(let* ((cb-info (new-schemesocbinfo))
       (triggerfield (new-sosftrigger))
       (syncoutsensor (new-sofieldsensor (get-scheme-sensor-cb)
                                         (void-cast cb-info))))
  (-> cb-info 'ref)
  (-> (-> cb-info 'callbackname) 'setvalue "syncout-callback")
  (-> triggerfield 'connectfrom (-> timecounter 'syncout))
  (-> syncoutsensor 'attach triggerfield))


;; FIXME: should have code for playing around with the
;; SoTimeCounter::duty field. 20000911 mortene.


;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))
(-> viewer-copy 'show)

;; Export scenegraph with engine.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with engine in it.
(let ((buffer "#Inventor V2.1 ascii\n\n Text3 { string \"X\" = TimeCounter { max 8 } . output }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Confirmed and potential bugs. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Reproduce Bugzilla #195: negative step values doesn't reverse the
;; counting order.
(-> (-> timecounter 'step) 'setValue -1)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Fixed bugs and false alarms (ex-possible bugs) ;;;;;;;;;;;;;;;;;;;;;;;;;

;; The counter doesn't restart at the modulo value of the step, but at
;; 0. This is confirmed to match the behavior of SGI Open Inventor.
(begin
  (-> (-> timecounter 'min) 'setValue 0)
  (-> (-> timecounter 'max) 'setValue 7)
  (-> (-> timecounter 'frequency) 'setValue 0.4)
  (-> (-> timecounter 'step) 'setValue 2))


;; Bugzilla #192: first turning the engine off, then back on -- engine
;; stops.
(-> (-> timecounter 'on) 'setValue 0) ; off
(-> (-> timecounter 'on) 'setValue 1) ; on


;; Setting the min and max values in the "wrong" order (so min>max) is
;; now handled by "clamping" min against max and vice versa.
(begin
  (-> (-> timecounter 'min) 'setValue 20)
  (-> (-> timecounter 'max) 'setValue 40)
  (-> (-> timecounter 'max) 'setValue 10)
  (-> (-> timecounter 'min) 'setValue 0))


;; Bugzilla #194: setting the value of SoTimeCounter::reset outside
;; the min-max range.
(begin ; Set up engine for test case.
  (-> (-> timecounter 'min) 'setValue 0)
  (-> (-> timecounter 'max) 'setValue 20)
  (-> (-> timecounter 'frequency) 'setValue 0.2)
  (-> (-> timecounter 'step) 'setValue 1))
;; Play around with this value to reproduce bug (update: now fixed).
(-> (-> timecounter 'reset) 'setValue 250)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> text 'justification) 'setValue SoText3::CENTER)
(-> (-> text 'parts) 'setValue SoText3::ALL)
(-> (-> text 'string) 'disconnect)
(-> viewer 'viewAll)
