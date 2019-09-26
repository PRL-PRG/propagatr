# propagatr

# Type Grammar

type := scalar
      | vector
      | matrix
      | class
      | env
      | expr

basetype := integer
          | character
          | complex
          | double
          | raw
          | logical

scalar := basteypte

vector := basetype[dim]

matrix := basteypte[dim, dim]

dim := _ | <number> | x

class := <string>

list := list[dim]

env := environment
     | environment<<type name ...>>

name := <symbol>

function_type := type, ... => type

!na, !null, null, missing

expr := expression

attr(names=type, id=type)

union        := |

intersection := &

externalptr

any

