import math
from scipy.optimize import least_squares, brentq
def triode(ug, ua, mu, ex, kg1, kp, kvb):
    x = kp*(1/mu + ug/math.sqrt(kvb + ua*ua))
    e1 = ua/kp*(x if x > 40 else math.log1p(math.exp(x)))
    return (e1**ex)/kg1 if e1 > 0 else 0.0
mu, kvb = 36.0, 300.0
def res(v):
    ex, lkg1, lkp = v
    kg1, kp = math.exp(lkg1), math.exp(lkp)
    f = lambda ug, ua: triode(ug, ua, mu, ex, kg1, kp, kvb)
    try:
        ia = brentq(lambda i: i - f(-i*240, 150), 1e-6, 0.1)
    except ValueError:
        return [10, 10]
    ug = -ia*240
    gm = (f(ug+1e-4, 150) - f(ug-1e-4, 150))/2e-4
    return [(ia-0.0085)/0.0085, (gm-0.0058)/0.0058]
best=None
for ex0 in (1.3,1.4,1.5):
  for kp0 in (100,300,600):
    r = least_squares(res, [ex0, math.log(400), math.log(kp0)], bounds=([1.2, -2, 2], [1.7, 14, 9]))
    if best is None or r.cost < best.cost: best = r
ex, kg1, kp = best.x[0], math.exp(best.x[1]), math.exp(best.x[2])
print('6N3P Ex %.4f Kg1 %.3f Kp %.3f residuals %s' % (ex, kg1, kp, [round(x,4) for x in best.fun]))
T = dict(mu=mu, ex=ex, kg1=kg1, kp=kp, kvb=kvb)
ia0 = brentq(lambda i: i - triode(-i*240, 150, **T), 1e-6, 0.1); ug0=-ia0*240
ra = 1/((triode(ug0, 151, **T)-triode(ug0, 149, **T))/2)
print('  op 150V/240R: Ia %.2f mA ug %.2f ra %.0f' % (ia0*1e3, ug0, ra))
for ua, ug in ((80, -1.6), (100, -1.25), (105, -1.4), (105, -1.3), (210, -1.3)):
    print('  UP-25 Ua %d ug %.2f -> Ia %.3f mA' % (ua, ug, triode(ug, ua, **T)*1e3))
