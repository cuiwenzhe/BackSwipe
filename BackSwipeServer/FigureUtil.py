#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jul 21 17:23:23 2020

@author: cuiwenzhe
"""
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import interp1d

def drawWordPoints(word,dict_word_points,centroids_X, centroids_Y):
    item = dict_word_points[word]
    x1 = item['raw_xs']
    y1 = item['raw_ys']
    x2 = item['normal_xs']
    y2 = item['normal_ys']
    
    fig, (ax1, ax2) = plt.subplots(2, 1)
    fig.suptitle('Horizontally stacked subplots') 
    ax1.invert_yaxis()
    ax2.invert_yaxis()
    ax1.plot(x1, y1)
    ax1.plot(centroids_X, centroids_Y, 'o')
    ax2.plot(x2, y2)

def drawPoints(nx, ny):
    fig, ax = plt.subplots()
    fig.suptitle('normalized gesture') 
    ax.invert_yaxis()
    ax.plot(nx, ny, 'bo')
    plt.show()
def drawOnPhoneScreen(xs, ys, x = 1080, y = 1920):
    fig, ax = plt.subplots(figsize=(9, 16))
    plt.axis([0, 1080, 0, 1920])
    ax.invert_yaxis()
    ax.plot(xs, ys, 'bo')
    plt.show()
def drawOnPhoneScreenWithCenter(xs, ys,cx,cy, x = 1080, y = 1920):
    fig, ax = plt.subplots(figsize=(9, 16))
    plt.axis([0, 1080, 0, 1920])
    ax.invert_yaxis()
    ax.plot(xs, ys, 'bo')
    ax.plot(cx, cy, 'ro')
    plt.show()
def drawWaveAndLine(wave, lines):
    fig, ax = plt.subplots()
    fig.suptitle('normalized gesture') 
    ax.plot(wave)
    plt.savefig('gesture_arc_gradients.png', dpi=200)
    plt.show()
def drawGrayDots(nx, ny, nc, name, scale = 10):
    fig, ax = plt.subplots()
    fig.suptitle('normalized gesture') 
    ax.invert_yaxis()
    ax.scatter(nx,ny,c = nc,s=scale)
    plt.gray()
    plt.savefig(name, dpi=200)
    plt.show()
    
def getMatrix(normal_xs,normal_ys, L = 100, is_square = True):
    normal_xs = normal_xs.astype('int')
    normal_ys = normal_ys.astype('int')
     #turn trace into figure
    width = np.max(normal_xs)+1
    height = np.max(normal_ys)+1
    pic_matrix = np.zeros([height,width])
    pic_matrix[normal_ys, normal_xs] = 10.0
    if not is_square:
        return pic_matrix
    else:
        half_gap = abs(height - width)//2
        square_pic = np.zeros([L,L])
        if width > height:
            square_pic[half_gap:pic_matrix.shape[0]+half_gap, 0:pic_matrix.shape[1]] = pic_matrix
        else:
            square_pic[0:pic_matrix.shape[0]:,half_gap:pic_matrix.shape[1]+half_gap] = pic_matrix
        return square_pic
'''
calculate the number of the hills in the signal
'''
def getHillNumber(ys):
    
    threshold = np.max(ys)/100
    peak_ys = []
    
    for i in range(1, len(ys) - 1):
        if (ys[i] - ys[i-1] > 0) and (ys[i] - ys[i+1] >= 0) and ys[i] > threshold:
            peak_ys.append(ys[i])
           
    count_hill = len(peak_ys)
    if len(peak_ys) != 0:
        if peak_ys[0] < ys[0]:
            count_hill += 1
        if peak_ys[-1] < ys[-1]:
            count_hill += 1
    
    return count_hill

'''
Get the envelope of a piece of signal
'''
def getEnvelope(ys, cmd='peak'):
    raw_peak_ys = [ys[0]]
    peak_ys = [ys[0]]
    peak_xs = [0]
    
    if len(ys) == 1:
        return np.array([0]),1
    
    for i in range(1, len(ys) - 1):
        if (ys[i] - ys[i-1] > 0) and (ys[i] - ys[i+1] >= 0):
            if (cmd == 'peak' or cmd == 'all'):
                peak_ys.append(ys[i])
                peak_xs.append(i)
                raw_peak_ys.append(ys[i])
        elif (ys[i] - ys[i-1] < 0) and (ys[i] - ys[i+1] <= 0):
            if (cmd == 'through' or cmd == 'all'):
                peak_ys.append(ys[i])
                peak_xs.append(i)
                raw_peak_ys.append(ys[i])
        else:
            raw_peak_ys.append(0)
    
    peak_ys.append(ys[-1])
    peak_xs.append(len(ys) - 1)
    raw_peak_ys.append(ys[-1])
    #print('peak_ys',peak_xs)
    fit_peak = interp1d(peak_xs,peak_ys, kind = 'quadratic',bounds_error = False, fill_value=0)    
    
    new_xs = np.array(range(len(ys)))
    fit_peak_ys = fit_peak(new_xs)
    return fit_peak_ys, getHillNumber(fit_peak_ys)