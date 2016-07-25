((nil . ((eval . (progn
                   (require 'projectile)
                   (puthash (projectile-project-root)
                            "make"
                            projectile-compilation-cmd-map)
		   (puthash (projectile-project-root)
                            "make test"
                            projectile-test-cmd-map))
	       ))))

