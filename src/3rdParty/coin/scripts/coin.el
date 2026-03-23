;;; coin.el --- miscellaneous code which simplifies some of the more
;;; mundane tasks when editing Coin sources in GNU Emacs.

;; Copyright (C) Kongsberg Oil & Gas Technologies.
;; Author: Morten Eriksen, <mortene@sim.no>.

;; This file is part of Coin.

;; The code present in this file is free software; you can
;; redistribute it and/or modify it under the terms of the GNU General
;; Public License version 2, as published by the Free Software
;; Foundation.

;; This code is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with Coin; see the file COPYING.  If not, see the www pages
;; of the GNU project at <http://www.gnu.org> for pointers to the full
;; license text.

;;; Commentary:

;; A grab-bag of miscellaneous functions to simplify some of the more
;; mundane tasks when editing Coin sourcecode from within Emacs. May
;; not be very useful for anyone but me yet, as they are under
;; development. :-}
;;
;; TODO:
;; o separate out the riff-raff from the code which is actually
;;   useful for others.
;;
;; 19990523 mortene.

;;; Code:

(defun coin-current-line ()
  "Return the current buffer line number and narrowed line number of point.
Code based on the standard Emacs WHAT-LINE function."
  (let ((opoint (point)) start)
    (save-excursion
      (save-restriction
	(goto-char (point-min))
	(widen)
	(beginning-of-line)
	(setq start (point))
	(goto-char opoint)
	(beginning-of-line)
	(1+ (count-lines 1 (point)))))))


(defun coin-make-header-define (filename)
  "Convert filename to a string suitable for C/C++ define checking."
  (if (not (equal (substring filename (- (length filename) 2)) ".h"))
      (message "Error: ``%s'' is not a valid filename" filename)
    (concat "COIN_"
	    (upcase (substring filename 0 (- (length filename) 2)))
	    "_H")))


(defun coin-remove-header-definecheck ()
  "Remove an ifndef/define/endif structure from a header file.
Returns point at beginning of line of starting #ifdef."
  (interactive)
  (save-excursion
    (beginning-of-buffer)
    (if (search-forward-regexp "^#if" (point-max) t)
	(let ((insertdefpt (match-beginning 0)))
	  (beginning-of-line)
	  (forward-line 1)
	  (if (equal (buffer-substring (point) (+ (point) 8)) "#define ")
	      (let (startpos)
		(forward-line 1)
		(setq startpos (point))
		(forward-line -2)
		(kill-region (point) startpos)
		(end-of-buffer)
		(search-backward-regexp "^#endif" (point-min) t)
		(beginning-of-line)
		(setq startpos (point))
		(end-of-buffer)
		(kill-region startpos (point))))
	  insertdefpt)
      0)))

;;;###autoload
(defun coin-set-header-definecheck ()
  "Execute in a header file to set the ifndef/define/endif structure.
If such a structure is already present, it will be removed, and a new
set will be made. The define will be constructed as follows:
``__basename_H__''."
  (interactive)
  (save-excursion
    (let ((ifpt (coin-remove-header-definecheck))
	  (dname (coin-make-header-define (file-name-nondirectory
					   buffer-file-name))))
      (goto-char ifpt)
      (insert "#ifndef " dname "\n")
      (insert "#define " dname "\n")
      (end-of-buffer)
      (insert "#endif // !" dname "\n"))))


(defun coin-insert-header-excludecheck ()
  "Insert an ifdef/error/endif block to check if a config rule is broken."
  (interactive)
  (progn
    (insert "#if defined(COIN_EXCLUDE_" (upcase (c++-classname)) ")\n")
    (insert "#error \"Configuration settings not respected, should not include this file!\"\n")
    (insert "#endif // COIN_EXCLUDE_" (upcase (c++-classname)) "\n")))





(defvar coin-overlay nil)


(defun coin-overlay-dehighlight ()
  (if coin-overlay
      (delete-overlay coin-overlay))
  (setq coin-overlay nil))


(defun coin-overlay-highlight (start end)
  (progn
    (if (not coin-overlay)
	(progn
	  (setq coin-overlay (make-overlay start end))
	  (overlay-put coin-overlay 'face 'highlight))
      (move-overlay coin-overlay start end))))


(defun coin-is-char-in-string (chr str)
  (let ((idx 0) (found nil))
    (while (and (< idx (length str))
		(not found))
      (if (equal chr (aref str idx))
	  (setq found t)
	(setq idx (+ 1 idx))))
    found))


(defun coin-make-operator-space (operatorchar
				 ignorebothsidechars
				 ignoreonesidechars)
  "Make sure there's space surrounding operators."
  (beginning-of-buffer)
  (let ((searchstr (concat ".\\" operatorchar ".")))
    (while (search-forward-regexp searchstr nil t)
      (let* ((mstr (match-string 0))
	     (prechr (aref mstr 0))
	     (postchr (aref mstr 2)))
	(if (and (not (coin-is-char-in-string prechr ignoreonesidechars))
		 (not (coin-is-char-in-string postchr ignoreonesidechars))
		 (or (not (coin-is-char-in-string prechr ignorebothsidechars))
		     (not (coin-is-char-in-string postchr ignorebothsidechars))))
	    (let ((start (- (point) 3)) (end (point)))
	      (coin-overlay-highlight start end)
	      (if (y-or-n-p "Fix? ")
		  (progn
		    (if (not (coin-is-char-in-string postchr ignorebothsidechars))
			(progn
			  (goto-char (- end 1))
			  (insert-char ?\  1)))
		    (if (not (coin-is-char-in-string prechr ignorebothsidechars))
			(progn
			  (goto-char (+ start 1))
			  (insert-char ?\  1)
			  (goto-char end)))))))))
    (coin-overlay-dehighlight)
    (message "")))


(defun coin-remove-redundant-comments ()
  "Remove larsa's redundant comments in the code. :-}"
  (beginning-of-buffer)
  (while (search-forward-regexp "[;\\\)\\\(}][ \t]*//.+$" nil t)
    (let ((start (+ (match-beginning 0) 1))
	  (end (match-end 0)))
      (coin-overlay-highlight start end)
      (if (y-or-n-p "Delete? ")
	  (delete-region start end))))
  (coin-overlay-dehighlight)
  (message ""))


(defun coin-remove-parentheses-space ()
  "Remove space after opening parentheses and before closing parentheses."
  (interactive)
  (save-excursion
    (beginning-of-buffer)
    (replace-string "( " "(")
    (beginning-of-buffer)
    (replace-string " )" ")")))


(defun coin-add-space-after-comma ()
  "Make sure there's always whitespace after a comma."
  (save-excursion
    (beginning-of-buffer)
    (while (search-forward-regexp ",[^ \t\n]" nil t)
      (let* ((mstr (match-string 0))
	     (postchr (aref mstr 1)))
	(delete-region (match-beginning 0) (match-end 0))
	(insert ", " postchr)))))


(defun coin-add-space-after-blockstart-keyword (keyword)
  "Add a space between a C/C++ keyword and a left parenthesis.
Keywords which should be sent to this method:
 if, while, ...
"
  (interactive)
  (save-excursion
    (beginning-of-buffer)
    (let ((rexp (concat "[ \t]+\\(" keyword "(\\)")))
      (while (search-forward-regexp rexp nil t)
	(delete-region (match-beginning 1) (match-end 1))
	(insert keyword " (")))))


;;;###autoload
(defun coin-format-code ()
  "Format the sourcecode according to rules in the coding style spec."
  (interactive)
  (save-excursion
    (condition-case ()
	(progn
					; Fully automatic
	  (coin-remove-parentheses-space)
          ; This proved to not be such a great idea, as it converted
          ; character constants ',' to ', ' in SbTime.cpp.
	  ; (coin-add-space-after-comma)
	  (coin-add-space-after-blockstart-keyword "if")
	  (coin-add-space-after-blockstart-keyword "while")
	  (coin-add-space-after-blockstart-keyword "for")
	  (coin-add-space-after-blockstart-keyword "switch")
					; With manual interaction
	  (coin-remove-redundant-comments)
	  (coin-make-operator-space "*" " *&/!¡<>),\"\\" "(")
	  (coin-make-operator-space "&" " *&<>),\"" "(")
	  (message "Code reformat done."))
      (quit (coin-overlay-dehighlight)))))



(defun coin-insert-debug-construct ()
  "Insert an #if .. SoDebugError::postInfo() .. #endif construct."
  (interactive)
  (beginning-of-line)
  (let ((startpos (point))
	(insstr (concat "#if COIN_DEBUG\n"
			"#include <Inventor/errors/SoDebugError.h>\n"
			"#endif // COIN_DEBUG\n\n")))
					; Insert debug construct
    (beginning-of-defun)
    (search-backward-regexp "::.+(" nil t)
    (beginning-of-line)
    (search-forward-regexp "^[^(]+" nil t)
    (goto-char startpos)
    (insert "#if COIN_DEBUG && 1 // debug\n"
	    "SoDebugError::postInfo(\"" (match-string 0) "\",\n"
	    "\"\");\n"
	    "#endif // debug\n")
    (forward-line -3)
    (c-indent-command)
    (forward-line 1)
    (c-indent-command)
    (forward-char 1)
    (setq startpos (point))
	    
					; Insert include, if necessary
    (if (not (search-backward-regexp "#include .*SoDebugError.h>" nil t))
	(progn
	  (search-backward-regexp "#include" nil t)
	  (search-forward-regexp "\n\n" nil t)
	  (beginning-of-line)
	  (insert insstr)
	  (setq startpos (+ startpos (length insstr)))))
    (goto-char startpos)))

(defun coin-flip-debug-on ()
  "Replace all ``COIN_DEBUG && 0'' with ``COIN_DEBUG && 1'' in buffer."
  (interactive)
  (save-excursion
    (beginning-of-buffer)
    (while (search-forward "COIN_DEBUG && 0" nil t)
      (replace-match "COIN_DEBUG && 1" nil t))))

(defun coin-flip-debug-off ()
  "Replace all ``COIN_DEBUG && 1'' with ``COIN_DEBUG && 0'' in buffer."
  (interactive)
  (save-excursion
    (beginning-of-buffer)
    (while (search-forward "COIN_DEBUG && 1" nil t)
      (replace-match "COIN_DEBUG && 0" nil t))))



(defun doxygen-class-doc ()
  (interactive)
  (let* ((fname (file-name-nondirectory buffer-file-name))
	 (classname (substring fname 0 (- (length fname) 4))))
    (insert "/*!\n"
	    "  \\class " classname " " classname ".h Inventor/Qt/devices/" classname ".h\n"
	    "  \\brief The " classname " class ...\n"
	    "  \\ingroup qtdevices\n"
	    "\n"
	    "  FIXME: write class doc\n"
	    "*/\n\n")

    (let ((rexp (concat classname "::" classname)))
      (if (search-forward-regexp rexp nil t)
	  (progn
	    (beginning-of-line)
	    (forward-line -1)
	    (insert "\n"
		    "/*!\n"
		    "  Constructor.\n"
		    "*/"))))

    (beginning-of-buffer)
    (let ((rexp (concat classname "::~" classname)))
      (if (search-forward-regexp rexp nil t)
	  (progn
	    (beginning-of-line)
	    (forward-line -1)
	    (insert "\n"
		    "/*!\n"
		    "  Destructor.\n"
		    "*/"))))
    
    (beginning-of-buffer)
    (let ((rexp (concat classname "::initClass")))
      (if (search-forward-regexp rexp nil t)
	  (progn
	    (beginning-of-line)
	    (forward-line -2)
	    (insert "\n"
		    "/*!\n"
		    "  Does initialization common for all objects of the\n"
		    "  " classname " class. This includes setting up the\n"
		    "  type system, among other things.\n"
		    "*/"))))
    
    (beginning-of-buffer)
    (let ((rexp (concat classname "::cleanClass")))
      (if (search-forward-regexp rexp nil t)
	  (progn
	    (beginning-of-line)
	    (forward-line -2)
	    (insert "\n"
		    "/*!\n"
		    "  Clean out all statically allocated resources.\n"
		    "  This method is only useful for debugging purposes.\n"
		    "*/"))))))


(defun doxygen-make-field-list ()
  "Returns a list of fields defined in a node class in Coin."
  (let ((rexp "^[ \t]*\\(So[SM]F[A-Za-z0-9]+\\)[ \t]+\\([a-zA-Z0-9]+\\)")
	(varlist ()))
    (while (search-forward-regexp rexp nil t)
      (setq varlist (cons (list (match-string 1) (match-string 2)) varlist)))
    (nreverse varlist)))

(defun doxygen-insert-fieldvar-doc ()
  (interactive)
  (let* ((cppbuf (buffer-name))
	 (basename (substring cppbuf 0 (- (length cppbuf) 4)))
	 (hbuf (concat basename ".h"))
	 (varlist ()))
    (set-buffer hbuf)
    (beginning-of-buffer)
    (setq varlist (doxygen-make-field-list))
    (set-buffer cppbuf)
    (while varlist
      (let ((elem (car varlist)))
	(insert "/*!\n"
		"  \\var " (car elem) " " basename "::" (car (cdr elem)) "\n"
		"  FIXME: write documentation for field\n"
		"*/\n"))
      (setq varlist (cdr varlist)))))


(defun c-find-enum ()
  "Returns a list with the name of the next enum and its definitions.

The format of the returned list is as follows:
  (``enumname'' (``definition0'' ``definition1'' ...))

Returns nil if no enum is found after current cursor position.
NB: running this function outside of a C/C++ header file does not
make much sense.

Note that regions that are commented out are also scanned, and
no preprocessing is done (so it won't work with enums defined with
any macro wizardry)."
  (let ((namerexp "^[ \t]*enum[ \t\n]+\\([^ \t\n{]+\\)")
	(enumlist ()))
    (if (search-forward-regexp namerexp nil t)
	(let ((deflist ()) (done))
	  (setq enumlist (match-string 1))
	  (skip-chars-forward " \t\n{")
	  (while (not done)
	    (if (search-forward-regexp "[ \t]*\\([^ \t\n,=}]+\\)")
		(progn
		  (setq deflist (cons (match-string 1) deflist))
		  (skip-chars-forward " \t\n")
		  (if (equal (following-char) ?\=)
		      (skip-chars-forward "^,}"))
		  (if (equal (following-char) ?\,)
		      (skip-chars-forward ", \t\n"))
		  (if (equal (following-char) ?\})
		      (setq done t)))
	      (setq done t)))
	  (setq enumlist (list enumlist (nreverse deflist)))))
    enumlist))


(defun c-make-enum-list ()
  "Returns a list of enums defined in a C/C++ header file."
  (interactive)
  (beginning-of-buffer)
  (let ((enumlist ()) (elem))
    (while (setq elem (c-find-enum))
      (setq enumlist (cons elem enumlist)))
    (nreverse enumlist)))


(defun c++-classname ()
  "Returns name of class which is either defined or implemented in buffer.
The class name is found by just extracting it from the filename, so this is
not exactly general purpose.."
  (let ((basename (file-name-nondirectory buffer-file-name))
	(classname ()))
    (when (equal (substring basename (- (length basename) 2)) ".h")
      (setq classname (substring basename 0 (- (length basename) 2))))
    (when (equal (substring basename (- (length basename) 4)) ".cpp")
      (setq classname (substring basename 0 (- (length basename) 4))))
    classname))
  

(defun doxygen-insert-enum-doc ()
  (interactive)
  (let* ((cppbuf (buffer-name))
	 (basename (substring cppbuf 0 (- (length cppbuf) 4)))
	 (hbuf (concat basename ".h"))
	 (varlist ()))
    (set-buffer hbuf)
    (beginning-of-buffer)
    (setq enumlist (c-make-enum-list))
    (set-buffer cppbuf)
    (while enumlist
      (let* ((elem (car enumlist))
	     (enumname (car elem))
	     (deflist (car (cdr elem))))
	(insert "/*!\n"
		"  \\enum " basename "::" enumname "\n"
		"  FIXME: write documentation for enum\n"
		"*/\n")
	(while deflist
	  (let ((delem (car deflist)))
	    (insert "/*!\n"
		    "  \\var " basename "::" enumname " " basename "::" delem "\n"
		    "  FIXME: write documentation for enum definition\n"
		    "*/\n")
	    (setq deflist (cdr deflist)))))
      (setq enumlist (cdr enumlist)))))


(defun doxygen-insert-fixme-function-block ()
  (interactive)
  (insert "/*!\n"
	  "  FIXME: write function documentation\n"
	  "*/\n"))

(defun doxygen-insert-assert-statement ()
  (interactive)
  (save-excursion
    (insert "assert(0 && \"FIXME: not implemented yet\");\n"))
  (c-indent-command))
