Implement options:

  * `-o <filename>` -- write current font to filename.
      * Implement writing fonts to files.
      * Implement this option.
  * `-O <filename>` -- write current font and unicode map to filename.
      * Implement writing fonts with a unicode map to a file.
      * Implement this option.
  * `-om <filename>` -- write current consolemap to filename.
      * Implement this option.
  * `-ou <filename>` -- write current unicodemap to filename.
      * Implement this option.
  * -{8|14|16} -- select a font from a codepage that contains three fonts.
      * Implement this option.
  * `-h<N>` (no space) -- override font height.
      * Implement this option.
  * `-m <fn>` -- load console screen map.
      * `-m none` -- suppress loading and activation of a screen map.
      * Implement this option.
  * `-u <fn>` -- load font unicode map.
      * `-u none` -- suppress loading of a unicode map.
      * Implement this option.
  * `-v` -- be verbose.
      * Investigate how it should behave.
      * Implement this option.
  * `-C <cons>` -- indicate console device to be used.
      * Implement this option.
  * `-V` -- print version and exit.
      * Implement this option.

---

256 perm: 4*x*x + 7*x + 42
