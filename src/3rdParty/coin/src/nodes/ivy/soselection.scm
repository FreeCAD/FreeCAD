;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-10-02.

;;; Eval following region

(define selection (new-soselection))

(-> selection 'addchild (new-socube))
(let ((trans (new-sotranslation)))
  (-> (-> trans 'translation) 'setvalue 5 0 0)
  (-> selection 'addchild trans))
(-> selection 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph selection)
(-> viewer 'setglrenderaction (new-soboxhighlightrenderaction))
(-> viewer 'redrawonselectionchange selection)
(-> viewer 'show)

;;; End initial eval-region


(define (selection-callback user-data selected-path)
  (format #t "You called, Master?~%"))

(define cb-info (new-schemesocbinfo))
(-> cb-info 'ref)
(-> (-> cb-info 'callbackname) 'setvalue "selection-callback")
(-> selection 'addselectioncallback
    (get-scheme-selection-path-cb)
    (void-cast cb-info))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test SoSelection node, play around ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> selection 'policy) 'setvalue SoSelection::SHIFT)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoSelection ;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))

;; Export scenegraph with SoSelection.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> viewer 'viewall)
