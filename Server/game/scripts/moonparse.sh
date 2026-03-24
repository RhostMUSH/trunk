#!/usr/bin/env python
# moonrise / moonset

from __future__ import division
from math import sqrt, pi, sin, cos, atan

def signum(x):
    if x < 0: return -1
    if x > 0: return 1
    return 0

def moon(B5, L5, H, Mo, D, Y):
    # lat, long, hours west of GMT, month, day, year
    # positive latitude is north, negative latitude is south
    # positive longitude is east, negative longitude is west
    # lat and long given in degrees and decimals of a degree
    
    # moon(38.6272, -90.1978, 6, 6, 30, 2014) # st louis, today
    
    # GOSUB 170 constants
    Ma = [[0 for x in range(4)] for y in range(4)]
        # we dimension to 4 instead of 3 because BASIC
        # arrays are 1-based and Python arrays are 0-based
    P2 = 2 * pi
    R1 = pi / 180
    K1 = 15 * R1 * 1.0027379
    L5 = L5 / 360
    Z0 = H / 24
    
    # GOSUB 760 calendar to julian date
    G = 1
    if Y < 1582: G = 0
    D1 = int(D); F = D - D1 - 0.5
    J = -1 * int(7 * (int((Mo+9)/12) + Y) / 4)
    J3 = 0 # not initialized in BASIC program
    if G <> 0:
        S = signum(Mo - 9)
        A = abs(Mo - 9)
        J3 = int(Y + S * int(A / 7))
        J3 = -1 * int((int(J3 / 100) + 1) * 3/4)
    J = J + int(275 * Mo / 9) + D1 + G * J3
    J = J + 1721027 + 2*G + 367*Y
    if F < 0:
        F = F + 1
        J = J - 1
    T = (J - 2451545) + F
    
    # GOSUB 245 lunar sidereal time at GMT time zone
    T0 = T / 36525
    S = 24110.5 + 8640184.813 * T0
    S = S + 86636.6 * Z0 + 86400 * L5
    S = S / 86400
    S = S - int(S)
    T0 = S * 360 * R1
    T = T + Z0
    
    # POSITION LOOP
    for I in range(1, 4):
        
        # GOSUB 495 fundamental arguments
        L = 0.606434 + 0.03660110129 * T
        M = 0.374897 + 0.03629164709 * T
        F = 0.259091 + 0.03674819520 * T
        D = 0.827362 + 0.03386319198 * T
        N = 0.347343 - 0.00014709391 * T
        G = 0.993126 + 0.00273777850 * T
        L = L - int(L); M = M - int(M)
        F = F - int(F); D = D - int(D)
        N = N - int(N); G = G - int(G)
        L = L * P2; M = M * P2; F = F * P2
        D = D * P2; N = N * P2; G = G * P2
        
        V = 0.39558 * sin(F + N)
        V = V + 0.08200 * sin(F)
        V = V + 0.03257 * sin(M - F - N)
        V = V + 0.01092 * sin(M + F + N)
        V = V + 0.00666 * sin(M - F)
        V = V - 0.00644 * sin(M + F - 2*D + N)
        V = V - 0.00331 * sin(F - 2*D + N)
        V = V - 0.00304 * sin(F - 2*D)
        V = V - 0.00240 * sin(M - F - 2*D - N)
        V = V + 0.00226 * sin(M + F)
        V = V - 0.00108 * sin(M + F - 2*D)
        V = V - 0.00079 * sin(F - N)
        V = V + 0.00078 * sin(F + 2*D + N)
        
        U = 1 - 0.10828 * cos(M)
        U = U - 0.01880 * cos(M - 2*D)
        U = U - 0.01479 * cos(2*D)
        U = U + 0.00181 * cos(2*M - 2*D)
        U = U - 0.00147 * cos(2*M)
        U = U - 0.00105 * cos(2*D - G)
        U = U - 0.00075 * cos(M - 2*D + G)
        
        W = 0.10478 * sin(M)
        W = W - 0.04105 * sin(2*F + 2*N)
        W = W - 0.02130 * sin(M - 2*D)
        W = W - 0.01779 * sin(2*F + N)
        W = W + 0.01774 * sin(N)
        W = W + 0.00987 * sin(2*D)
        W = W - 0.00338 * sin(M - 2*F - 2*N)
        W = W - 0.00309 * sin(G)
        W = W - 0.00190 * sin(2*F)
        W = W - 0.00144 * sin(M + N)
        W = W - 0.00144 * sin(M - 2*F - N)
        W = W - 0.00113 * sin(M + 2*F + 2*N)
        W = W - 0.00094 * sin(M - 2*D + G)
        W = W - 0.00092 * sin(2*M - 2*D)
        
        # compute right ascension, declination, distance
        S = W / sqrt(U - V*V)
        A5 = L + atan(S / sqrt(1 - S*S))
        S = V / sqrt(U); D7 = atan(S / sqrt(1 - S*S))
        R5 = 60.40974 * sqrt(U)
        Ma[I][1] = A5
        Ma[I][2] = D7
        Ma[I][3] = R5
        T = T + 0.5
        
    if Ma[2][1] <= Ma[1][1]: Ma[2][1] = Ma[2][1] + P2
    if Ma[3][1] <= Ma[2][1]: Ma[3][1] = Ma[3][1] + P2
    Z1 = R1 * (90.567 - 41.685/Ma[2][3])
    S = sin(B5 * R1); C = cos(B5 * R1)
    Z = cos(Z1); M8 = 0; W8 = 0
    A0 = Ma[1][1]; D0 = Ma[1][2]
    V0 = 0 # not initialized in BASIC program
    
    for C0 in range(0, 24):
        
        P = (C0 + 1) / 24
        
        F0 = Ma[1][1]; F1 = Ma[2][1]; F2 = Ma[3][1]
        A = F1 - F0; B = F2 - F1 - A
        F = F0 + P * (2*A + B*(2*P-1))
        A2 = F
        
        F0 = Ma[1][2]; F1 = Ma[2][2]; F2 = Ma[3][2]
        A = F1 - F0; B = F2 - F1 - A
        F = F0 + P * (2*A + B*(2*P-1))
        D2 = F
        
        # GOSUB 285 test an hour for an event
        L0 = T0 + C0 * K1; L2 = L0 + K1
        if A2 <> 0: A2 = A2 + 2*pi
        H0 = L0 - A0; H2 = L2 - A2
        H1 = (H2 + H0) / 2 # hour angle
        D1 = (D2 + D0) / 2 # declination
        if C0 <= 0:
            V0 = S * sin(D0) + C * cos(D0) * cos(H0) - Z
        V2 = S * sin(D2) + C * cos(D2) * cos(H2) - Z
        if signum(V0) <> signum(V2):
            V1 = S * sin(D1) + C * cos(D1) * cos(H1) - Z
            A = 2*V2 - 4*V1 + 2*V0; B = 4*V1 - 3*V0 - V2
            D = B*B - 4*A*V0
            if D >= 0:
                D = sqrt(D)
                if V0 < 0 and V2 > 0: print "MOONRISE AT",
                if V0 < 0 and V2 > 0: M8 = 1
                if V0 > 0 and V2 < 0: print "MOONSET AT",
                if V0 > 0 and V2 < 0: W8 = 1
                E = (-1*B + D) / (2 * A)
                if E > 1 or E < 0:
                    E = (-1*B - D) / (2 * A)
                T3 = C0 + E + 1/120
                H3 = int(T3); M3 = int((T3 - H3) * 60)
                print H3, ":", M3,
                H7 = H0 + E * (H2 - H0)
                N7 = -1 * cos(D1) * sin(H7)
                D7 = C * sin(D1) - S * cos(D1) * cos(H7)
                A7 = atan(N7 / D7) / R1
                if D7 < 0: A7 = A7 + 180
                if A7 > 360: A7 = A7 - 360
                print ", AZ", A7
        A0 = A2; D0 = D2; V0 = V2
        
    # GOSUB 450 special message
    if M8 <> 0 or W8 <> 0:
        if M8 == 0: print "NO MOONRISE THIS DATE"
        if W8 == 0: print "NO MOONSET THIS DATE"
    else:
        if V2 <> 0: print "MOON UP ALL DAY"

moon(38.6272, -90.1978, 6, 6, 30, 2014)
