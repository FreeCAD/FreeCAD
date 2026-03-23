s/ *SO_SFIELD_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  SO_SFIELD_CONSTRUCTOR_HEADER(\1);\
  SO_SFIELD_REQUIRED_HEADER(\1);\
  SO_SFIELD_VALUE_HEADER(\1, \2, \3)/g
s/ *SO_SFIELD_DERIVED_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  SO_SFIELD_CONSTRUCTOR_HEADER(\1);\
  SO_SFIELD_REQUIRED_HEADER(\1); \
  SO_SFIELD_DERIVED_VALUE_HEADER(\1, \2, \3)/g
s/ *SO_MFIELD_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  SO_SFIELD_CONSTRUCTOR_HEADER(\1);\
  SO_SFIELD_REQUIRED_HEADER(\1); \
  SO_MFIELD_VALUE_HEADER(\1, \2, \3)/g
s/ *SO_MFIELD_DERIVED_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  SO_SFIELD_CONSTRUCTOR_HEADER(\1);\
  SO_SFIELD_REQUIRED_HEADER(\1); \
  SO_MFIELD_DERIVED_VALUE_HEADER(\1, \2, \3)/g
s/ *SO_SFIELD_CONSTRUCTOR_HEADER(\([^)]*\))/public:\
  \1(void);\
  virtual ~\1()/g
s/ *SO_SFIELD_REQUIRED_HEADER(\([^)]*\))/private:\
  static SoType classTypeId;\
public:\
  static void * createInstance(void);\
  static SoType getClassTypeId(void);\
  virtual SoType getTypeId(void) const;\
  virtual void copyFrom(const SoField \& field);\
  const \1 \& operator=(const \1 \& field);\
  virtual SbBool isSame(const SoField \& field) const/g
s/ *SO_SFIELD_DERIVED_VALUE_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  PRIVATE_SFIELD_IO_HEADER();\
public:\
  \3 operator=(\3 newvalue)/g
s/ *SO_SFIELD_VALUE_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  PRIVATE_SFIELD_IO_HEADER();\
protected:\
  \2 value;\
public:\
  \3 getValue(void) const;\
  void setValue(\3 newvalue);\
  \3 operator=(\3 newvalue);\
  int operator==(const \1 \& field) const;\
  int operator!=(const \1 \& field) const/g
s/ *PRIVATE_SFIELD_IO_HEADER(*)/private:\
  virtual SbBool readValue(SoInput * in);\
  virtual void writeValue(SoOutput * out) const/g
s/ *SO_MFIELD_VALUE_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  PRIVATE_MFIELD_IO_HEADER();\
protected:\
  virtual void deleteAllValues(void);\
  virtual void copyValue(int to, int from);\
  virtual int fieldSizeof(void) const;\
  virtual void * valuesPtr(void);\
  virtual void setValuesPtr(void * ptr);\
  virtual void allocValues(int num);\
  \2 * values;\
public:\
  \3 operator[](const int idx) const; \
  const \2 * getValues(const int start) const; \
  int find(\3 value, SbBool addifnotfound = FALSE); \
  void setValues(const int start, const int num, const \2 * newvals); \
  void set1Value(const int idx, \3 value); \
  void setValue(_valref_ value); \
  \3 operator=(_valref_ val);\
  SbBool operator==(const _class_ \& field) const;\
  SbBool operator!=(const _class_ \& field) const;\
  \2 * startEditing(void);\
  void finishEditing(void)/g
s/ *SO_MFIELD_DERIVED_VALUE_HEADER( *\([^,]*\), *\([^,]*\), *\([^)]*\))/  PRIVATE_MFIELD_IO_HEADER();\
public:\
  \3 operator=(\3 val)/g
s/ *PRIVATE_MFIELD_IO_HEADER(.*)/private:\
  virtual SbBool read1Value(SoInput * in, int idx);\
  virtual void write1Value(SoOutput * out, int idx) const/g
s/ *SO_MFIELD_SETVALUESPOINTER_HEADER(\([^)]*\))/  void setValuesPointer(const int num, const \1 * userdata);\
  void setValuesPointer(const int num, \1 * userdata)/g
