%extend SoFullPath {
  /* allow construction of an SoFullPath through the provision of an
   * SoPath, to remedy the need for a cast operator. */
  static SoFullPath * fromSoPath(SoPath * path) {
    return static_cast<SoFullPath *>(path);
  }
}
