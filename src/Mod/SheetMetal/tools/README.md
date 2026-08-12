# Air Bending

![air-bending-punch-distances](./air-bending-punch-distances.svg)


```
    <- b -> <- a ->
----+------+-------|
    B      A
```

## First bend (A)

x is equal to the "ML" distance, which is "a" in this case.

## After first bend (B, ...)

x is equal to "b + FD" distance. Calculate FD value with [calc-unfold](./calc-unfold.py) tool.

## K-factor table

Taken from https://en.wikipedia.org/wiki/Bending_(metalworking):

| Generic K-factors (ANSI)   | Aluminum       | Aluminum         | Steel          |
|----------------------------|----------------|------------------|----------------|
| Radius                     | Soft materials | Medium materials | Hard materials |
| **Air bending**            |                |                  |                |
| 0 to thickness             | 0.33           | 0.38             | 0.40           |
| Thickness to 3 × thickness | 0.40           | 0.43             | 0.45           |
| Greater than 3 × thickness | 0.50           | 0.50             | 0.50           |
| **Bottoming**              |                |                  |                |
| 0 to thickness             | 0.42           | 0.44             | 0.46           |
| Thickness to 3 × thickness | 0.46           | 0.47             | 0.48           |
| Greater than 3 × thickness | 0.50           | 0.50             | 0.50           |
| **Coining**                |                |                  |                |
| 0 to thickness             | 0.38           | 0.41             | 0.44           |
| Thickness to 3 × thickness | 0.44           | 0.46             | 0.47           |
| Greater than 3 × thickness | 0.50           | 0.50             | 0.50           |
