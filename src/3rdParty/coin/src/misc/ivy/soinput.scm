;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-20.

;; Import scenegraph from file.
(let ((input (new-soinput)))
  (-> input 'openfile "/home/sigma/mortene/foot01.iv")
  (let* ((sceneroot (sodb::readall input))
         (bonematerial (sobase::getnamedbase (new-sbname "bone") (sonode::getclasstypeid))))
    (display "\n")
    (display "sceneroot: ")
    (display sceneroot)
    (display "\n")
    (display "bone: ")
    (display bonematerial)
    (display "\n")
    ))

(define hepp (new-solabel))
(-> hepp 'setname (new-sbname "hepp"))
(sobase::getnamedbase (new-sbname "hepp") (sonode::getclasstypeid))
