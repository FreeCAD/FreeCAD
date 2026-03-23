;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-11.

;;; Eval following region

(define root (new-sogroup))
(define text3 (new-sotext3))
(-> root 'addchild text3)

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoText3 node, play around with these ;;;;;;;;;;;;;;

;; SoText3::parts
(-> (-> text3 'parts) 'setvalue SoText3::FRONT)
(-> (-> text3 'parts) 'setvalue SoText3::SIDES)
(-> (-> text3 'parts) 'setvalue SoText3::BACK)
(-> (-> text3 'parts) 'setvalue (+ SoText3::BACK SoText3::FRONT))
(-> (-> text3 'parts) 'setvalue (+ SoText3::SIDES SoText3::FRONT))
(-> (-> text3 'parts) 'setvalue SoText3::ALL)

;; SoText3::string
(begin
  (set-mfield-values! (-> text3 'string) 0
                      (map new-sbstring '("tjo" "bing" "!!")))
  (-> viewer 'viewall))

;; SoText3::justification
(-> (-> text3 'justification) 'setvalue SoText3::CENTER) ; LEFT/RIGHT/CENTER

;; SoText3::spacing
(begin
  (-> (-> text3 'spacing) 'setvalue 0.7)
  (-> viewer 'viewall))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test material bindings on SoText3 text ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define materials (new-somaterial))
(-> root 'insertchild materials 0)

(define diffusecolors (-> materials 'diffusecolor))
(set-mfield-values! diffusecolors 0
                    '(
                      #(1 0 0)
                      #(1 1 0)
                      #(1 1 1)
                      #(0 1 1)
                      #(0 0 1)
                      ))

(define materialbinding (new-somaterialbinding))
(-> root 'insertchild materialbinding 0)
;; Available bindings: OVERALL/PER_PART/PER_PART_INDEXED/PER_FACE/
;;                     PER_FACE_INDEXED/PER_VERTEX/PER_VERTEX_INDEXED
(-> (-> materialbinding 'value) 'setvalue SoMaterialBinding::PER_PART)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test profiling of SoText3 text ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Evaluate these two expressions first to test with an empty profile.
(define linearprofile (new-solinearprofile))
(-> root 'insertchild linearprofile 0)

;; Add profile coordinate node.
(define profile2coords (new-soprofilecoordinate2))
(-> root 'insertchild profile2coords 0)

(define coords2field (-> profile2coords 'point))
(set-mfield-values! coords2field 0
                    '(
                      ; For the first SoLinearProfile.
                      #(0.00 0.00)
                      #(1.00 0.75)
                      #(2.00 0.5)
                      #(3.00 0.75)
                      #(4.00 0.00)

                      ; For the second SoLinearProfile.
                      #(5.00 0.00)
                      #(6.00 0.75)
                      #(7.00 0.5)
                      #(8.00 0.75)
                      #(9.00 0.00)
                      ))

;; Try second set of coordinates first. FIXME: bugs
(set-mfield-values! (-> linearprofile 'index) 0 '(5 6 7 8 9))
;; Use first set.
(set-mfield-values! (-> linearprofile 'index) 0 '(0 1 2 3 4))

;; Add the second SoLinearProfile.
(define linearprofile2 (new-solinearprofile))
(-> (-> linearprofile2 'linkage) 'setvalue SoProfile::ADD_TO_CURRENT)
(-> root 'insertchild linearprofile2 2)
(set-mfield-values! (-> linearprofile2 'index) 0 '(5 6 7 8 9))

;; Use only the second SoLinearProfile.
(-> (-> linearprofile2 'linkage) 'setvalue SoProfile::START_FIRST)

;; Remove second SoLinearProfile.
(-> root 'removechild linearprofile2)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Change font settings ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define font (new-sofont))
(-> root 'insertchild font 0)
(-> (-> font 'size) 'setvalue 5)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Change complexity settings ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define complexity (new-socomplexity))
(-> root 'insertchild complexity 0)
(-> (-> complexity 'value) 'setvalue 1.0)

(define shapehints (new-soshapehints))
(-> root 'insertchild shapehints 0)
(-> (-> shapehints 'creaseangle) 'setvalue 0.8)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoText3 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> root 'copy 1))

;; Export scenegraph.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(-> viewer 'viewall)
(-> viewer 'setscenegraph (new-socube))
