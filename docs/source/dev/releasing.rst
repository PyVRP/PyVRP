Releasing
=========

This page explains the steps that need to be taken to release a new version of PyVRP to the Python package index (PyPI).
Before proceeding, make sure you are on ``main`` and everything builds correctly, and the tests pass.

Regular release
---------------

1. Bump the version number in the ``pyproject.toml`` file to the version you are about to release.
   For example, set ``version = "0.12.0"``.

2. Create a local tag with the version number prefixed by ``v``. For example, ``v0.12.0``.

3. Checkout the new tag:

   .. code-block:: shell

      git checkout v0.12.0

4. Build a distribution to ensure everything works well: 

   .. code-block:: shell

      uv build

   Make sure that the source distribution and wheel built like this in ``dist/`` have the correct version in their names!

5. Push the tag to GitHub:

   .. code-block:: shell

      git push origin tag v0.12.0

6. On GitHub, go into `releases <https://github.com/PyVRP/PyVRP/releases>`_ and draft a new release using the appropriate release notes.
   Make sure to choose the tag we just pushed as the content to be released.
   Choose the tag name for the release title.

7. Release the new version.
   This should automatically trigger the CD workflow to build wheels, and push those to PyPI.
   Make sure to check the CD and PyPI to ensure everything went well.

8. After the release is on PyPI, amend the release notes with a link to the Zenodo DOI automatically assigned to this release.
   The release-specific DOI can be found on PyVRP's `Zenodo entry <https://doi.org/10.5281/zenodo.11409402>`_.

9. **[SKIP IF PATCH]** Push the latest version of the docs to `the archive repository <https://github.com/PyVRP/PyVRP.github.io>`_.
   The latest docs can be downloaded from the ``DOC`` workflow associated with the released tag's commit.
   After pushing to the archive repository, update the version info in ``docs/source/versions.json`` to include a link to the newly archived documentation.

10. **[SKIP IF PATCH]** Bump the version in ``pyproject.toml`` to the next prerelease, *e.g.* from ``0.12.0`` to ``0.13.0a0``.
    Then commit this to ``main``, together with the updates to ``docs/source/versions.json``. 


Releasing a patch
-----------------

Sometimes it is necessary to quickly release several bugfixes via a patch, bumping from *e.g.* ``0.12.0`` to ``0.12.1``.
This can be done as follows:

1. Checkout the existing tag we are going to patch.
   For example:

   .. code-block:: shell

      git checkout v0.12.0

2. Create a new, temporary branch here:

   .. code-block:: shell

      git checkout -b <BRANCH NAME>

3. Cherry pick the commits from ``main`` that need to go into the patch.
   For example:

   .. code-block:: shell

      git cherry-pick <COMMIT SHA1> <COMMIT SHA2>

4. Since ``main`` may have changed significantly since the release of ``v0.12.0``, ensure that the new cherry-picked commits work by recompiling and running the tests.
   If something does not work, fix it now.

5. Once everything works, commit and proceed as with a regular release.
   Make sure to delete the temporary branch once you have pushed your new tag to GitHub.
