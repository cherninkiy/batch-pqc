from estimator import *

def SVP_Classical(dim):
	return 8*(dim)*(3/2)**(dim/2)

def SVP_Quantum(dim):
	return 8*(dim)*ZZ(2)**RR(0.265*dim)


# LWR
def LWR_estiamtes(n, l, k, q, p, s, model=SVP_Classical):
	nLWE = n*l
	m = k*n # m = k*n
	s_bound = s
	secret_distribution = (-s_bound, s_bound)
	alpha = sqrt(2*pi)*1/RR(2*p)
	success_probability = 0.99
	reduction_cost_model =  lambda beta, d, B: model(beta)
	res_primal = primal_usvp(nLWE, alpha, q, secret_distribution=secret_distribution, m=m, success_probability=success_probability, reduction_cost_model=reduction_cost_model)
	res_dual = dual_scale(nLWE, alpha, q, secret_distribution=secret_distribution, m=m, success_probability=success_probability, reduction_cost_model=reduction_cost_model)
	return res_primal, res_dual

# Testing SIS
# This analysis is given in Dilithium, C3
# https://eprint.iacr.org/eprint-bin/getfile.pl?entry=2017/633&version=20180910:112250&file=633.pdf


# go from beta to delta for BKZ
def get_delta(beta):
	return RR(beta/(2*pi*e) * (pi*beta)**(1/beta))**(1/(2*(beta-1)))

def getGSA(d, q, beta, nrows):
	delta = get_delta(beta)
	log_delta =log(delta,2)

	log_q = log(q)
	zone1 = nrows*[log_q]
	slope = -(1/(beta-1))*log(beta/(2*pi*e)*(pi*beta)^(1/beta))
	zone2_length = int(floor(log(q)/-slope))
	zone2 = [log_q + i * slope for i in range(1, zone2_length+1)]
	zone3 =(d-nrows)*[0]
	GSA = zone1+zone2+zone3

	lattice_vol = nrows*log_q
	current_vol = sum(GSA[i] for i in range(d))

	#correct the volume since now current_vol > lattice_vol
	ind = 0
	while current_vol>lattice_vol:
		current_vol -= GSA[ind]
		current_vol += GSA[ind+d]
		ind += 1
	GSA = GSA[ind:ind+d]

	i_index = max(0, nrows-ind)
	j_index = min(i_index + zone2_length, d)
	#error we make by overshooting the while loop
	err = lattice_vol - current_vol
	for i in range(i_index, j_index):
		GSA[i] += err / (j_index-i_index+1)

	return GSA, i_index, j_index


def getGSA_noq(d, q, beta, nrows):
	delta = get_delta(beta)
	log_delta =log(delta,2)

	log_q = log(q)
	slope = -(1/(beta-1))*log(beta/(2*pi*e)*(pi*beta)^(1/beta))

	lattice_vol = nrows*log_q
	current_vol = 0
	GSA_tmp = 0
	GSA = []
	for i in range(d):
			GSA_tmp -= slope
			current_vol += GSA_tmp
			if lattice_vol<current_vol:
				break
			GSA = [GSA_tmp] + GSA

	zone2_length = len(GSA)
	GSA += (d-zone2_length)*[0]
	i_index = 0
	j_index = min(i_index + zone2_length, d)

	#error we make by overshooting the while loop
	current_vol = sum(GSA[i] for i in range(d))
	err = current_vol - lattice_vol
	for i in range(i_index, j_index):
		GSA[i] -= err / (j_index-i_index+1)

	return GSA, i_index, j_index



def RunTime(beta, lat_dim, q, nrows, B, model=SVP_Classical):

	GSA, i_index, j_index = getGSA(lat_dim, q, beta, nrows)
	#GSA, i_index, j_index = getGSA(lat_dim, q, beta, nrows)
	assert i_index>=0, 'i_index>=0'
	assert j_index<=len(GSA), 'j_index<=N'
	assert i_index!=j_index, 'i_index!=j_index'

	Pr1 = (2.*B)/q
	log_Pr1 = ((i_index)*(log(Pr1, 2)))

	norm_BKZ = exp(GSA[i_index])
	sigma = norm_BKZ / sqrt(j_index - i_index + 1)
	Pr2 = erf(B / (sigma * sqrt(2.)))
	log_Pr2 =  (j_index-i_index+1)* log(Pr2, 2)

	logPr_total_ = (beta/2.0)*log(4./3., 2) + log_Pr1+log_Pr2
	logPr_total = min(0,logPr_total_)
	log_runTimeSVP = log(model(beta),2).n()
	log_runTime = log_runTimeSVP - logPr_total

	return [i_index, j_index, round(log_Pr1+log_Pr2), log_runTimeSVP, round(log_runTime)]


def SIS_esimates(n, l, k, q, B, model=SVP_Classical):
	d = n*(k + l + 1)
	m = n*k
	best_rt = 10000
	beta_range = range(100, d, 10)
	for beta in beta_range:
		if (log(model(beta),2) > best_rt):
			break
		for w in [d]:
			lat_dim = w
			log_runtime = RunTime(beta, lat_dim, q, m, B, model=model)
			if(log_runtime[4]<best_rt):
				best_rt = log_runtime[4]
				best_beta = beta
				best_param = log_runtime
	return best_beta, best_rt, best_param

print('===================== GOOSEBERRY 1 =====================')
n = 256
l = 2
k = 3
nu = 23
mu = 19
q = 2^nu
p = 2^mu

w = 20 # weight of c
s = 4  # inf bound on s
d = 3  # number of MSB's of Ay
beta = w * s # inf bound on c*s
gamma = 1048576 # inf bound on y


# 2nd preimage resistance for the hash function
R = 2^w * binomial(n, w)
print('log R:', log(R, 2).n())


p1 = (1 - beta/(gamma - 0.5))^(n*l)   # probability that IF passes on z-conditions
p2 = (1 - (w*2^(nu-mu+2))/(2^(nu-d)-1))^(n*k)  # probability that IF passes on LSB-conditions
nRestarts = 1/(p1*p2)

print('p1:', p1, 'p2:', p2.n(), 'nRestarts:', nRestarts)

# Harndess of SIS: norm of vectors SIS should find to break the scheme
norms_sSIS = [gamma - beta, 2^(nu-d)+w*2^(nu-mu)]
norms_SIS  = [2^(nu-d+1), 2*(gamma-beta)]
B = max(norms_SIS+norms_SIS)
assert B<(q/2), 'B is too big'
print('SIS infinity bound: ', B, 'q/B',  (q/B).n())


print('----------- classical hardness -----------')
LWR_Hardness_c = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Classical)
print('LWR primal: ', LWR_Hardness_c[0])
print('LWR dual: ', LWR_Hardness_c[1])
SIS_Hardness_c = SIS_esimates(n, l, k, q, B, model=SVP_Classical)
print('SIS runtime:',SIS_Hardness_c[1], 'beta: ', SIS_Hardness_c[0] )


print('----------- quantum hardness -----------')
LWR_Hardness_q = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Quantum)
print('LWR primal: ', LWR_Hardness_q[0])
SIS_Hardness_q = SIS_esimates(n, l, k, q, B, model=SVP_Quantum)
print('SIS runtime:',SIS_Hardness_q[1], 'beta: ', SIS_Hardness_q[0])


print('===================== GOOSEBERRY 2 =====================')
n = 256
l = 3
k = 4
nu = 23
mu = 19
q = 2^nu
p = 2^mu

w = 39 # weight of c
s = 4  # inf bound on s
d = 3  # number of MSB's of Ay
beta = w * s # inf bound on c*s
gamma = 1048576 # inf bound on y


# 2nd preimage resistance for the hash function
R = 2^w * binomial(n, w)
print('log R:', log(R, 2).n())


p1 = (1 - beta/(gamma - 0.5))^(n*l)   # probability that IF passes on z-conditions
p2 = (1 - (w*2^(nu-mu+2))/(2^(nu-d)-1))^(n*k)  # probability that IF passes on LSB-conditions
nRestarts = 1/(p1*p2)

print('p1:', p1, 'p2:', p2.n(), 'nRestarts:', nRestarts)

# Harndess of SIS: norm of vectors SIS should find to break the scheme
norms_sSIS = [gamma - beta, 2^(nu-d)+w*2^(nu-mu)]
norms_SIS  = [2^(nu-d+1), 2*(gamma-beta)]
B = max(norms_SIS+norms_SIS)
assert B<(q/2), 'B is too big'
print('SIS infinity bound: ', B, 'q/B',  (q/B).n())


print('----------- classical hardness -----------')
LWR_Hardness_c = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Classical)
print('LWR primal: ', LWR_Hardness_c[0])
print('LWR dual: ', LWR_Hardness_c[1])
SIS_Hardness_c = SIS_esimates(n, l, k, q, B, model=SVP_Classical)
print('SIS runtime:',SIS_Hardness_c[1], 'beta: ', SIS_Hardness_c[0] )


print('----------- quantum hardness -----------')
LWR_Hardness_q = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Quantum)
print('LWR primal: ', LWR_Hardness_q[0])
SIS_Hardness_q = SIS_esimates(n, l, k, q, B, model=SVP_Quantum)
print('SIS runtime:',SIS_Hardness_q[1], 'beta: ', SIS_Hardness_q[0])


print('===================== GOOSEBERRY 3 =====================')
n = 256
l = 4
k = 5
nu = 23
mu = 19
q = 2^nu
p = 2^mu

w = 50 # weight of c
s = 3  # inf bound on s
d = 3  # number of MSB's of Ay
beta = w * s # inf bound on c*s
gamma = 1048576 # inf bound on y


# 2nd preimage resistance for the hash function
R = 2^w * binomial(n, w)
print('log R:', log(R, 2).n())


p1 = (1 - beta/(gamma - 0.5))^(n*l)   # probability that IF passes on z-conditions
p2 = (1 - (w*2^(nu-mu+2))/(2^(nu-d)-1))^(n*k)  # probability that IF passes on LSB-conditions
nRestarts = 1/(p1*p2)

print('p1:', p1, 'p2:', p2.n(), 'nRestarts:', nRestarts)

# Harndess of SIS: norm of vectors SIS should find to break the scheme
norms_sSIS = [gamma - beta, 2^(nu-d)+w*2^(nu-mu)]
norms_SIS  = [2^(nu-d+1), 2*(gamma-beta)]
B = max(norms_SIS+norms_SIS)
assert B<(q/2), 'B is too big'
print('SIS infinity bound: ', B, 'q/B',  (q/B).n())


print('----------- classical hardness -----------')
LWR_Hardness_c = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Classical)
print('LWR primal: ', LWR_Hardness_c[0])
print('LWR dual: ', LWR_Hardness_c[1])
SIS_Hardness_c = SIS_esimates(n, l, k, q, B, model=SVP_Classical)
print('SIS runtime:',SIS_Hardness_c[1], 'beta: ', SIS_Hardness_c[0] )


print('----------- quantum hardness -----------')
LWR_Hardness_q = LWR_estiamtes(n, l, k, q, p, s, model=SVP_Quantum)
print('LWR primal: ', LWR_Hardness_q[0])
SIS_Hardness_q = SIS_esimates(n, l, k, q, B, model=SVP_Quantum)
print('SIS runtime:',SIS_Hardness_q[1], 'beta: ', SIS_Hardness_q[0])
