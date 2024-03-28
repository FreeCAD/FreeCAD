#pragma once
// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

// GoogleTest's ASSERT_NO_THROW() doesn't let us show any info about the exceptions.
// This wrapper macro will output the e57::E57Exception context on failure.
#define E57_ASSERT_NO_THROW( code )                                                                \
   try                                                                                             \
   {                                                                                               \
      code;                                                                                        \
   }                                                                                               \
   catch ( e57::E57Exception & err )                                                               \
   {                                                                                               \
      FAIL() << err.errorStr() << ": " << err.context();                                           \
   }

#define E57_ASSERT_THROW( code ) ASSERT_THROW( code, e57::E57Exception )

// For readability of preprocessor using E57_VALIDATION_LEVEL
#define VALIDATION_OFF 0
#define VALIDATION_BASIC 1
#define VALIDATION_DEEP 2

#define VALIDATE_BASIC ( E57_VALIDATION_LEVEL > VALIDATION_OFF )
#define VALIDATE_DEEP ( E57_VALIDATION_LEVEL > VALIDATION_BASIC )
