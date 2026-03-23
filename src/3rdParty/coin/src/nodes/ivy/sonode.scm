;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (C) Kongsberg Oil & Gas Technologies. All rights reserved.
;;; Written by mortene@sim.no, 2000-09-18.

(let ((root (new-sogroup))
      (topgroup-0 (new-sogroup))
      (topgroup-1 (new-sogroup))
      (child-group (new-sogroup)))
  ;; Set up scenegraph.
  (-> root 'addchild topgroup-0)
  (-> root 'addchild topgroup-1)
  (-> topgroup-0 'addchild child-group)
  (-> topgroup-1 'addchild child-group)
  (-> child-group 'addchild (new-socone))
  ;; 
  (let ((scenecopy (-> root 'copy 1)))
    ()))

        (writeaction (new-sowriteaction)))
    (-> writeaction 'apply root)))
    
