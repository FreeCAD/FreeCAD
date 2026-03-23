;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-10-02.

;;; Eval following region

(define extselection (new-soextselection))

(-> extselection 'addchild (new-socube))
(let ((trans (new-sotranslation)))
  (-> (-> trans 'translation) 'setvalue 5 0 0)
  (-> extselection 'addchild trans))
(-> extselection 'addchild (new-socone))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph extselection)
(-> viewer 'setglrenderaction (new-soboxhighlightrenderaction))
(-> viewer 'redrawonselectionchange extselection)
(-> viewer 'show)

;;; End initial eval-region


(define (selection-callback user-data selected-path)
  (format #t "You called, Master?~%"))

(define cb-info (new-schemesocbinfo))
(-> cb-info 'ref)
(-> (-> cb-info 'callbackname) 'setvalue "selection-callback")
(-> extselection 'addselectioncallback
    (get-scheme-selection-path-cb)
    (void-cast cb-info))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test SoExtSelection node, play around ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> extselection 'lassotype) 'getvalue)
(-> (-> extselection 'lassotype) 'setvalue SoExtSelection::RECTANGLE)
(-> (-> extselection 'lassopolicy) 'getvalue)
(-> (-> extselection 'lassopolicy) 'setvalue SoExtSelection::PART_BBOX)
(-> extselection 'isusingoverlay)
(-> extselection 'useoverlay 0)
(-> extselection 'getlassowidth)
(-> extselection 'setlassowidth 10)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoExtSelection ;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))

;; Export scenegraph with SoExtSelection.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> viewer 'viewall)
(-> extselection 'lassopolicy)
(-> extselection 'lassotype)
(soextselection::lassopolicy extselection)
(soextselection::lassotype extselection)
