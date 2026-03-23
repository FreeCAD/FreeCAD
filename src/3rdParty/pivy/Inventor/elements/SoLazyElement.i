/* workaround for wrong const int * -> const uint32_t * in
   SoLazyElement::setPacked() */
%ignore setPacked(SoState * state, SoNode * node,
                  int32_t numcolors, const uint32_t * colors,
                  const SbBool packedtransparency = FALSE);

%extend SoLazyElement {
  void setPacked(SoState * state, SoNode * node,
                 int32_t numcolors, const uint32_t * colors,
                 const SbBool packedtransparency = FALSE) {
    SoLazyElement::setPacked(state, node, numcolors, colors, packedtransparency);
  }
}
