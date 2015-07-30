
Each subdirectory contains a dockerfile that does something useful:

* ``cyclus-deps`` builds all cyclus dependencies.  This is used as the base
  image for building cyclus and should be updated as needed and pushed up to
  the docker hub ``cyclus/cyclus-deps`` repository.
