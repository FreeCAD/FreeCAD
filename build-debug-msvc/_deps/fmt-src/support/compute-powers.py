#!/usr/bin/env python
# Compute 10 ** exp with exp in the range [min_exponent, max_exponent] and print
# normalized (with most-significant bit equal to 1) significands in hexadecimal.

from __future__ import print_function

min_exponent = -348
max_exponent = 340
step = 8
significand_size = 64
exp_offset = 2000

class fp:
    pass

powers = []
for i, exp in enumerate(range(min_exponent, max_exponent + 1, step)):
    result = fp()
    n = 10 ** exp if exp >= 0 else 2 ** exp_offset / 10 ** -exp
    k = significand_size + 1
    # Convert to binary and round.
    binary = '{:b}'.format(n)
    result.f = (int('{:0<{}}'.format(binary[:k], k), 2) + 1) / 2
    result.e = len(binary) - (exp_offset if exp < 0 else 0) - significand_size
    powers.append(result)
    # Sanity check.
    exp_offset10 = 400
    actual = result.f * 10 ** exp_offset10
    if result.e > 0:
        actual *= 2 ** result.e
    else:
        for j in range(-result.e):
            actual /= 2
    expected = 10 ** (exp_offset10 + exp)
    precision = len('{}'.format(expected)) - len('{}'.format(actual - expected))
    if precision < 19:
        print('low precision:', precision)
        exit(1)

print('Significands:', end='')
for i, fp in enumerate(powers):
    if i % 3 == 0:
        print(end='\n ')
    print(' {:0<#16x}'.format(fp.f, ), end=',')

print('\n\nExponents:', end='')
for i, fp in enumerate(powers):
    if i % 11 == 0:
        print(end='\n ')
    print(' {:5}'.format(fp.e), end=',')

print('\n\nMax exponent difference:',
      max([x.e - powers[i - 1].e for i, x in enumerate(powers)][1:]))
