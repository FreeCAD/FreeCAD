;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-27.

;;; Eval following region

(define root (new-sogroup))
(define texture2 (new-sotexture2))
(-> root 'addchild texture2)
(-> root 'addchild (new-socube))

(define viewer (new-soxtexaminerviewer))
(-> viewer 'setscenegraph root)
(-> viewer 'show)

;;; End initial eval-region


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Test input fields of SoTexture2 node, play around with these ;;;;;;;;;;;

(-> (-> texture2 'filename)
    'setvalue (new-sbstring "/home/sigma/mortene/pix/champadam.jpg"))
(-> (-> texture2 'filename)
    'setvalue (new-sbstring "/home/sigma/mortene/pix/06.jpg"))
(-> (-> texture2 'filename)
    'setvalue (new-sbstring "/home/sigma/mortene/pix/dime1.psd"))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Misc operations on the graph with the SoTexture2 ;;;;;;;;;;;;;;;;;;;;;;;;;

;; Copy the scenegraph.
(define viewer-copy (new-soxtexaminerviewer))
(-> viewer-copy 'show)
(-> viewer-copy 'setscenegraph (-> root 'copy 1))

;; Export scenegraph with SoTexture2.
(define writeaction (new-sowriteaction))
(-> writeaction 'apply (-> viewer 'getscenegraph))

;; Read scenegraph with SoTexture2 in it.
(let ((buffer "#Inventor V2.1 ascii\n\nSeparator { Texture2 {
 	 image 8 6 3
	 0xff000000 0xff000000 0x00000000 0x00000000 0xff00ff00 0xff00ff00
	 0xff000000 0xff000000 0x00000000 0x00000000 0xff00ff00 0xff00ff00
	 0xff000000 0xff000000 0x00000000 0x00000000 0xff00ff00 0xff00ff00
	 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
	 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
	 0xffff0000 0xffff0000 0x00000000 0x00000000 0xffffff00 0xffffff00
	 0xffff0000 0xffff0000 0x00000000 0x00000000 0xffffff00 0xffffff00
	 0xffff0000 0xffff0000 0x00000000 0x00000000 0xffffff00 0xffffff00
 }  Sphere { } }")
      (input (new-soinput)))
  (-> input 'setbuffer (void-cast buffer) (string-length buffer))
  (let ((sceneroot (sodb::readall input)))
    (-> viewer 'setscenegraph sceneroot)
    (-> viewer 'viewall)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Scratch area ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(-> viewer 'viewall)
