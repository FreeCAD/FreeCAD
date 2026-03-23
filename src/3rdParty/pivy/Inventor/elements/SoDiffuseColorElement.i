/* workaround for wrong const int * -> const uint32_t * in
   SoDiffuseColorElement::set() */
%ignore set(SoState * const state, SoNode * const node,
            const int32_t numcolors, const uint32_t * const colors,
            const SbBool packedtransparency = FALSE);

%extend SoDiffuseColorElement {
  void set(SoState * const state, SoNode * const node,
           const int32_t numcolors, const uint32_t * const colors,
           const SbBool packedtransparency = FALSE) {
    SoDiffuseColorElement::set(state, node, numcolors, colors, packedtransparency);
  }
}
