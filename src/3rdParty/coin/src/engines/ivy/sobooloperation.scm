;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-13.

;;; Eval following region

;; Return vector with 3D dimensions of root subgraph.
(define (subgraph-size root)
  (let ((bboxaction (new-sogetboundingboxaction (new-sbviewportregion))))
    (-> bboxaction 'apply root)
    (let* ((bbox (-> bboxaction 'getboundingbox))
           (mincorner (-> bbox 'getmin))
           (maxcorner (-> bbox 'getmax)))
      (-> (-> maxcorner 'operator- mincorner) 'getvalue))))

;; Append translation node to root node's set of children.
(define (append-translation root translationvalue)
  (-> root 'addchild
      (let ((transnode (new-sotranslation)))
        (-> (-> transnode 'translation) 'setvalue translationvalue)
        transnode)))


;; Make scene graph.
(define root (new-soseparator))

(define text-a (new-sotext3))
(-> (-> text-a 'string) 'setValue "FALSE")
(-> root 'addchild text-a)

(append-translation root
                    (vector (* 1.5 (vector-ref (subgraph-size text-a) 0))
                            0.0 0.0))

(define text-b (new-sotext3))
(-> (-> text-b 'string) 'setValue "FALSE")
(-> root 'addchild text-b)

(append-translation root
                    (vector (* 1.5 (vector-ref (subgraph-size text-b) 0))
                            0.0 0.0))

(define text-ops (new-sotext3))
(-> (-> text-ops 'string) 'setValue "NOT_A_AND_NOT_B")
(-> root 'addchild text-ops)
(append-translation root
                    (vector (* 1.5 (vector-ref (subgraph-size text-ops) 0))
                            0.0 0.0))

(define text-result (new-sotext3))
(-> (-> text-result 'string) 'setValue "FALSE")
(-> root 'addchild text-result)
(append-translation root
                    (vector (* 1.5 (vector-ref (subgraph-size text-result) 0))
                            0.0 0.0))

(define text-inverse (new-sotext3))
(-> (-> text-inverse 'string) 'setValue "FALSE")
(-> root 'addchild text-inverse)

(define booloperation (new-sobooloperation))

(-> (-> text-a 'string) 'connectFrom (-> booloperation 'a))
(-> (-> text-b 'string) 'connectFrom (-> booloperation 'b))
(-> (-> text-ops 'string) 'connectFrom (-> booloperation 'operation))
(-> (-> text-result 'string) 'connectFrom (-> booloperation 'output))
(-> (-> text-inverse 'string) 'connectFrom (-> booloperation 'inverse))

;; Make viewer.
(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoBooloperation engine, play around ;;;;;;;;;;;;;;;;;

(-> (-> booloperation 'a) 'setValue 0)
(-> (-> booloperation 'b) 'setValue 0)
;; operations:
;;       CLEAR, SET,
;;       A, NOT_A,
;;       B, NOT_B,
;;       A_OR_B, NOT_A_OR_B, A_OR_NOT_B, NOT_A_OR_NOT_B,
;;       A_AND_B, NOT_A_AND_B, A_AND_NOT_B, NOT_A_AND_NOT_B,
;;       A_EQUALS_B, A_NOT_EQUALS_B
(-> (-> booloperation 'operation) 'setValue SoBoolOperation::CLEAR)

(set-mfield-values! (-> booloperation 'a) 0 '(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0))
(set-mfield-values! (-> booloperation 'b) 0 '(1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1))
(set-mfield-values! (-> booloperation 'operation) 0
                    (list SoBoolOperation::CLEAR
                          SoBoolOperation::SET
                          SoBoolOperation::ENUM_A
                          SoBoolOperation::NOT_A
                          SoBoolOperation::ENUM_B
                          SoBoolOperation::NOT_B
                          SoBoolOperation::A_OR_B
                          SoBoolOperation::NOT_A_OR_B
                          SoBoolOperation::A_OR_NOT_B
                          SoBoolOperation::NOT_A_OR_NOT_B
                          SoBoolOperation::A_AND_B
                          SoBoolOperation::NOT_A_AND_B
                          SoBoolOperation::A_AND_NOT_B
                          SoBoolOperation::NOT_A_AND_NOT_B
                          SoBoolOperation::A_EQUALS_B
                          SoBoolOperation::A_NOT_EQUALS_B ))
                       
;; Test with "missing" values.
(-> (-> booloperation 'a) 'setnum 1)
(-> (-> booloperation 'b) 'setnum 1)
(-> (-> booloperation 'operation) 'setnum 0)


;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'setscenegraph (-> (-> viewer 'getscenegraph) 'copy 1))
(-> viewer-copy 'show)

;; Export scenegraph with engine.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with engine in it.
(let ((buffer "#Inventor V2.1 ascii\n\n Text3 { string \"X\" = BoolOperation { a TRUE  b FALSE  operation NOT_A_OR_B } . output }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Confirmed and potential bugs. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; False alarms (ex-possible bugs) and ex-bugs ;;;;;;;;;;;;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> (-> text 'justification) 'setValue SoText3::CENTER)
(-> (-> text 'string) 'disconnect)
(-> viewer 'viewAll)

;; Import scenegraph with engine from file.
(let ((input (new-soinput)))
  (-> input 'openfile "/home/sigma/mortene/tmp/boolop.iv")
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))
