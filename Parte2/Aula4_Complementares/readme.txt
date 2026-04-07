Os arquivos que representam as instancias para a Mochila 01 tem o seguinte formato:

0/1 Knapsack problem files
File name: kp_n_wmax
kp: name of the instance of 0/1 knapsack problem
n: number of items
wmax: knapsack capacity

Example
n wmax
v1 w1
v2 w2
: :
vi wi
: :
vn wn

vi: profit of item i
wi: weight of item i

-------------------------------------------------------------------

Os arquivos que representam as instancias para o TSP tem o seguinte formato:

Algumas informações iniciais em relação às n cidades que podem ser ignoradas.

Cada cidade é representada por uma coordenada cartesiana (x, y), então a
distancia entre duas cidades é definida pela distância euclidiana.

Seja, x[i] e y[i] as coordenadas da cidade i. 
A distancia dij entre cidades i e j é calculada assim:
xd = x[i]- x[j]; 
yd = y[i]- y[j]; 
dij = nint( sqrt( xd*xd + yd*yd) );

Onde a função “nint” (“nint(x)” pode ser trocada por “(int) (x+0.5)”).