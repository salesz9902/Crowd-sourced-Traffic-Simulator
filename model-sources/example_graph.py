import numpy as np;
import graph_tool.all as gt
import scipy as SP
import matplotlib.pyplot as plt

#G is a simple road network in Example 2 (see Figure 2)
G = gt.Graph(directed=True)

v1 = G.add_vertex()
v2 = G.add_vertex()
v3 = G.add_vertex()
v4 = G.add_vertex()
v5 = G.add_vertex()

e1 = G.add_edge(v1, v2)
e2 = G.add_edge(v2, v1)
e3 = G.add_edge(v2, v3)
e4 = G.add_edge(v3, v4)
e5 = G.add_edge(v4, v5)
e6 = G.add_edge(v5, v2)
e7 = G.add_edge(v2, v4)
e8 = G.add_edge(v4, v2)

vlen = len(G.get_vertices())

#Figure 2
gt.graph_draw(G, vertex_text=G.vertex_index, vertex_font_size=18,output_size=(800, 800), output="example2.pdf")

#A will be the adjacency matrix of G (4)
A = gt.adjacency(G)

with open('log.txt', 'w') as file_w:
	file_w.write("A= (4)\n")
	file_w.write(str(A.todense().transpose()))
	file_w.write("\n")

#Example 10
L = gt.laplacian(G, deg='total', normalized=False, weight=None, index=None) # unnormalized Laplace of Example 10
L = L-A.transpose() #L (46)

with open('log.txt', 'ab') as file_w:
	file_w.write("L= (46 left)\n")
	file_w.write(str(L.todense()))
	file_w.write("\n")

#Linv = np.linalg.pinv(L.todense()) #Moore-Penrose inverse (48)

#with open('Linv.txt', 'w') as file_w:
#	file_w.write(str(Linv))

#Example 12
N = np.array([[0,250,0,0,0], [450,0,200,150,0], [0,0,0,450,0], 
               [0,200,0,0,300], [0,350,0,0,0]]);  # 2-dimensional frequencies (75)

print(N.shape)

P_uncorr = np.array([[0,1,0,0,0], [0.5625,0,0.25,0.1875,0], [0,0,0,1,0], 
               [0,0.4,0,0,0.6], [0,1,0,0,0]]);	#2-dimensional probabilities (uncorrected)

with open('log.txt', 'ab') as file_w:
	file_w.write("Example 12\nN= (75)\n")
	file_w.write(str(N))
	file_w.write("\n")

diff = np.transpose(np.array([[-200,0,250,-100,50]])); # s-e
print(diff.shape)

with open('log.txt', 'ab') as file_w:
	file_w.write("diff=\n")
	file_w.write(str(diff))
	file_w.write("\n")

#lamb = np.dot(Linv,diff);

lamb = np.linalg.lstsq(L.todense(), diff)
lamb = lamb[0]

with open('log.txt', 'ab') as file_w:
	file_w.write("lamb=\n")
	file_w.write(str(lamb))
	file_w.write("\n")

one = np.ones((5,1));

R = np.multiply(np.dot(one,np.transpose(lamb))-np.dot(lamb,np.transpose(one)),A.todense().transpose());

with open('log.txt', 'ab') as file_w:
	file_w.write("R=\n")
	file_w.write(str(R))
	file_w.write("\n")

Q = N+R;  # 2-dimensional stationary frequencies

with open('log.txt', 'ab') as file_w:
	file_w.write("N+R\n")
	file_w.write(str(Q))
	file_w.write("\n")

Qdist = Q/(sum(Q).sum());  # 2-dimensional stationary distribution

with open('log.txt', 'ab') as file_w:
	file_w.write("Q=\n")
	file_w.write(str(Qdist))
	file_w.write("\n")

pi = sum(Qdist);  # stationary distribution

with open('log.txt', 'ab') as file_w:
	file_w.write("pi=\n")
	file_w.write(str(pi))
	file_w.write("\n")

P = np.transpose(np.divide(np.transpose(Qdist),pi));

with open('log.txt', 'ab') as file_w:
	file_w.write("P=\n")
	file_w.write(str(P))
	file_w.write("\n")
