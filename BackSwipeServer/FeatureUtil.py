#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jul 21 17:22:06 2020

@author: cuiwenzhe
"""
import numpy as np
from PIL import Image, ImageDraw

# def getSegments(gradients, low_bound = 10, high_bound = 50):
#     low_count = 0
#     high_count = 0
#     for i in range(1, len(gradients)):
#         v1 = abs(gradients[i-1])
#         v2 = abs(gradients[i])
#         if v2 >= high_bound and v1 < high_bound:
#             high_count += 1
#         if v2 >= low_bound and v1 < low_bound:
#             low_count += 1
            
#     low_count += 1
#     high_count += 1
    
#     res = list(range(high_count-1, low_count+1))
#     if res == [] or max(res) > 12:
#         res = list(range(1,20))
#     return res
def getCorners(gradients, high_bound = 50):
    high_count = 0
    for i in range(1, len(gradients)):
        v1 = abs(gradients[i-1])
        v2 = abs(gradients[i])
        if v2 >= high_bound and v1 < high_bound:
            high_count += 1
    
    # high_count += 1
    
    res = list(range(max(high_count-2,0), max(high_count-2,0)+4))
    # if high_count >= low_count:
    #     high_count = max(high_count,2)
    #     res = [high_count-1, high_count, high_count+1]
    if max(res) >= 7:
        res = list(range(0,12))
    return res
def getSegments(gradients, low_bound = 20, high_bound = 50):
    low_count = 0
    high_count = 0
    for i in range(1, len(gradients)):
        v1 = abs(gradients[i-1])
        v2 = abs(gradients[i])
        if v2 >= high_bound and v1 < high_bound:
            high_count += 1
        if v2 >= low_bound and v1 < low_bound:
            low_count += 1
            
    low_count += 1
    high_count += 1
    
    res = list(range(max(high_count-2,1), low_count+2))
    if high_count >= low_count:
        high_count = max(high_count,2)
        res = [high_count-1, high_count, high_count+1]
    if max(res) > 12:
        res = list(range(1,20))
    return res

def getPicture(xs,ys, stroke_width=3, L = 100):
    xs = np.array(xs)
    ys = np.array(ys)
    xs = xs.astype('float')
    ys = ys.astype('float')
    #xs, ys = generateSamplePoints(xs, ys)
    # xs = np.array(xs)
    # ys = np.array(ys)
    xs = xs.astype(int)
    ys = ys.astype(int)
     #turn trace into figure
    min_xs = min(xs)
    max_xs = max(xs)
    min_ys = min(ys)
    max_ys = max(ys)
    
    width = max_xs - min_xs + 1
    height  = max_ys - min_ys + 1
    xs = xs - min_xs
    ys = ys - min_ys

    path = list(zip(xs,ys))
    
    img = Image.new('L',(width, height))
    draw = ImageDraw.Draw(img)
    draw.line(path, fill=255, width = stroke_width, joint = 'curve')
    return img
def getArcTans(xs, ys):
    diff_xs = np.ediff1d(xs, to_begin=0)
    diff_ys = np.ediff1d(ys, to_begin=0)
    return np.arctan2(diff_ys, diff_xs)
def getDegrees(xs, ys):
    diff_xs = np.ediff1d(xs, to_begin=xs[1]-xs[0])
    diff_ys = np.ediff1d(ys, to_begin=ys[1]-ys[0])
    arct2 = np.arctan2(diff_ys, diff_xs)
    return np.degrees(arct2)
def getGradient(xs,ys):
    gradx = np.gradient(np.gradient(xs))
    grady = np.gradient(np.gradient(ys))
    return gradx, grady
def getDensity(xs, ys):
    x_steps = np.ediff1d(xs, xs[1]-xs[0])
    y_steps = np.ediff1d(ys, ys[1]-ys[0])
    point_steps = np.sqrt(x_steps**2 + y_steps**2)
    density = point_steps
    return density
def getSpeeds(xs, ys, timestamps):
    x_steps = np.ediff1d(xs, to_begin=0)
    y_steps = np.ediff1d(ys, to_begin=0)
    time_gaps = np.ediff1d(timestamps, to_begin = 1)
    dists = np.hypot(x_steps, y_steps)
    speeds = dists*10/time_gaps
    # print("dist_gaps:" + str(dists))
    # print("time_gaps:" + str(time_gaps))
    # print("speeds:" + str(speeds))
    nan_mask = np.isnan(speeds)
    inf_mask = np.isinf(speeds)
    mask = np.logical_or(nan_mask, inf_mask)
    speeds[mask] = np.interp(np.flatnonzero(mask), np.flatnonzero(~mask), speeds[~mask])
    return speeds
    
def getGradient2(xs, ys):
    grad2x = np.gradient(np.gradient(xs))
    grad2y = np.gradient(np.gradient(ys))
    return grad2x, grad2y

# def getFeatures(xs, ys):
#     grad2s = getGradient2(xs, ys)
#     mean_x = np.mean(xs)
#     mean_y = np.mean(ys)
#     std_x = np.std(xs)
#     std_y = np.std(ys)
#     mean_grad2s = np.mean(grad2s)
#     std_grad2s = np.std(grad2s)
#     return np.array([mean_x, mean_y, std_x, std_y, mean_grad2s, std_grad2s])
def getFeatures(xs, ys):
    complex_points = np.append(xs, ys)
    return complex_points
    #return np.append(flat_points,grad2s)