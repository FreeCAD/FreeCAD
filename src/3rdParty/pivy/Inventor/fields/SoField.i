%ignore SoField::get(SbString & valuestring);

%extend SoField {
  SbString get() {
    SbString valuestring;
    self->get(valuestring);
    return valuestring;
  }

}
