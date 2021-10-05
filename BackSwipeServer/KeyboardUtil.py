#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jul 21 17:09:54 2020

@author: cuiwenzhe
"""
import numpy as np
from scipy.interpolate import interp1d



DEFAULT_KEYBOARD_WIDTH = 1200
DEFAULT_KEYBOARD_HEIGHT = 900
KEY_WIDTH = DEFAULT_KEYBOARD_WIDTH/10
KEY_HEIGHT = DEFAULT_KEYBOARD_HEIGHT/3

# the x/y coordinate of the center of keys
keyXArr_Touchpad = [
	0.5 * KEY_WIDTH, 5.5 * KEY_WIDTH, 3.5 * KEY_WIDTH, 2.5 * KEY_WIDTH, 2 * KEY_WIDTH, # a~e
	3.5 * KEY_WIDTH, 4.5 * KEY_WIDTH, 5.5 * KEY_WIDTH, 7.0 * KEY_WIDTH, 6.5 * KEY_WIDTH, # f-j
	7.5 * KEY_WIDTH, 8.5 * KEY_WIDTH, 7.5 * KEY_WIDTH, 6.5 * KEY_WIDTH, 8.0 * KEY_WIDTH, # k-o
	9.0 * KEY_WIDTH, 0.0 * KEY_WIDTH, 3.0 * KEY_WIDTH, 1.5 * KEY_WIDTH, 4.0 * KEY_WIDTH, # p - t
	6.0 * KEY_WIDTH, 4.5 * KEY_WIDTH, 1.0 * KEY_WIDTH, 2.5 * KEY_WIDTH, 5.0 * KEY_WIDTH, 1.5 * KEY_WIDTH  # u - z
    ]

keyYArr_Touchpad = [
	1.5 * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 1.5 * KEY_HEIGHT, 0.5 * KEY_HEIGHT, # a~e
	1.5 * KEY_HEIGHT, 1.5 * KEY_HEIGHT, 1.5 * KEY_HEIGHT, 0.5 * KEY_HEIGHT, 1.5 * KEY_HEIGHT, # f-j
	1.5 * KEY_HEIGHT, 1.5 * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 0.5 * KEY_HEIGHT, # k -o
	0.5 * KEY_HEIGHT, 0.5 * KEY_HEIGHT, 0.5 * KEY_HEIGHT, 1.5 * KEY_HEIGHT, 0.5 * KEY_HEIGHT, # p - t
	0.5 * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 0.5 * KEY_HEIGHT, 2.5  * KEY_HEIGHT, 0.5 * KEY_HEIGHT, 2.5 * KEY_HEIGHT # u - z
    ]
centroids_X = np.array(keyXArr_Touchpad) + np.ones(len(keyXArr_Touchpad)) * KEY_WIDTH * 0.5
centroids_Y = np.array(keyYArr_Touchpad)

exp_keyXArr = np.array([279.6, 497.5, 382.0, 341.6, 338.2, # a~e
               382.4, 416.6, 494.2, 560.4, 565.0, # f~j
               554.1, 624.6, 593.1, 511.8, 608.9, # k~o
               624.1, 214.2, 390.6, 308.0, 406.6, # p~t
               498.3, 441.0, 282.3, 380.2, 458.9,243.4]) # u~z


exp_keyYArr = np.array([692.4, 757.3, 762.2, 713.4, 619.0, # a~e
               739.9, 691.7, 704.5, 635.1, 695.2,  # f~j
               712.1, 684.4, 801.6, 765.4, 605.0,  # k~o
               572.3, 621.8, 625.4, 717.3, 597.7, # p~t
               626.5, 766.0, 630.7, 702.4, 581.4, 890.2]) # u~z

import math

def getWordLenth(word):
    count = 0
    cur = 0
    for c in word:
        if c != cur:
            count += 1
        cur = c
    return count
def getUnrepeatWord(word):
    nword = ''
    cur = 0
    for c in word:
        if c != cur:
            nword += c
        cur = c
    return nword

def getThreePointDegree(a, b, c):
    a = np.array(a)
    b = np.array(b)
    c = np.array(c)
    ba = a - b
    bc = c - b
    
    cosine_angle = np.dot(ba, bc) / (np.linalg.norm(ba) * np.linalg.norm(bc))
    angle = np.arccos(cosine_angle)
    return np.degrees(angle)

def getSharpWordCorner(rword, degree_threshold):
    word = getUnrepeatWord(rword)
    if getWordLenth(word) == 1:
       return -1
    if getWordLenth(word) == 2:
       return 0
    count = 0
    for i in range(1,len(word)-1):
        c1 = word[i-1]
        c2 = word[i]
        c3 = word[i+1]
        p1 = [centroids_X[ord(c1)-97],centroids_Y[ord(c1)-97]]
        p2 = [centroids_X[ord(c2)-97],centroids_Y[ord(c2)-97]]
        p3 = [centroids_X[ord(c3)-97],centroids_Y[ord(c3)-97]]
        if getThreePointDegree(p1,p2,p3) < degree_threshold:
            count += 1                  
    return count

def getWordSegment(rword):
    word = getUnrepeatWord(rword)
    if getWordLenth(word) == 1:
       return 0
    if getWordLenth(word) == 2:
       return 1
    count = 0
    for i in range(1,len(word)-1):
        c1 = word[i-1]
        c2 = word[i]
        c3 = word[i+1]
        c1x = centroids_X[ord(c1)-97]
        c1y = centroids_Y[ord(c1)-97]
        c2x = centroids_X[ord(c2)-97]
        c2y = centroids_Y[ord(c2)-97]
        c3x = centroids_X[ord(c3)-97]
        c3y = centroids_Y[ord(c3)-97]
        d1x = c2x - c1x
        d1y = c2y - c1y
        d2x = c3x - c2x
        d2y = c3y - c2y
        if not ((d1y == 0 and d2y == 0 and d1x*d2x > 0) or ((d1x == 0 and d2x == 0 and d1y*d2y > 0))):
            count += 1                  
    return count+1

def distance(x1, y1, x2, y2):
    return math.hypot(x1 - x2, y1 - y2)

def get_template_sample_points(xs, ys, count=300):
    """
    Resample a gesture template of a word into N equidistant points.
    :param word: target word
    :param count: number of corresponding gesture samples, we will sample the
    template into same number of points.
    :return: list of indices of the sample points, which correspond to each
    letter in the word.
    """
    # template_points = get_sample_points(xs, ys, count, "template")

    # get the index of the sample points which are closest to key centers
    indices = [0]
    dists = []
    length = 0
    for i in range(1, len(xs)):
        d = distance(xs[i - 1], ys[i - 1], xs[i], ys[i])
        dists.append(d)
        length += d
    step = length / (count - 1) # count points, (count - 1) segments

    length = 0
    for i in range(0, len(dists) - 1):
        length += dists[i]
        # round to nearest int, then convert to int
        indices.append(int(round(length / step, 0)))

    # add the last point's index (avoid rounding errors -> out of bound)
    indices.append(count - 1)
    return indices

def getTemplateLetterCenters(word):
    cen_xs = []
    cen_ys = []
    for c in word:
        cen_xs.append(centroids_X[ord(c)-97])
        cen_ys.append(centroids_Y[ord(c)-97])
    return cen_xs, cen_ys

def getGestureKeyCenters(word,sample_xs, sample_ys):
    tx, ty = getTemplateLetterCenters(word)
    indices = get_template_sample_points(tx, ty)
    return (np.array(sample_xs)[indices], np.array(sample_ys)[indices])

def getRelativeDxDy(x,y,letter,keyboard_width, keyboard_height):
    letter_x = centroids_X[ord(letter)-97]*keyboard_width/DEFAULT_KEYBOARD_WIDTH
    letter_y = centroids_Y[ord(letter)-97]*keyboard_height/DEFAULT_KEYBOARD_HEIGHT
    return letter_x, letter_y
    
def getDxDy(word,gesture_letter_xs, gesture_letter_ys, keyboard_width, keyboard_height):
    template_letter_ori_xs, template_letter_ori_ys = getTemplateLetterCenters(word)
    template_letter_xs = (np.array(template_letter_ori_xs)*keyboard_width/DEFAULT_KEYBOARD_WIDTH)
    template_letter_ys = (np.array(template_letter_ori_ys)*keyboard_height/DEFAULT_KEYBOARD_HEIGHT)
    dxs =  np.mean(gesture_letter_xs - template_letter_xs)      
    dys =  np.mean(gesture_letter_ys - template_letter_ys)
    return dxs, dys

def getKeyboardOriMean(keyboard_width, keyboard_height):
    ox = np.mean(centroids_X * keyboard_width/DEFAULT_KEYBOARD_WIDTH)
    oy = np.mean(centroids_X * keyboard_height/DEFAULT_KEYBOARD_HEIGHT)
    return ox,oy

def transformSampledTemplate(txs,tys,dx,dy,keyboard_width, keyboard_height):
    txs = np.array(txs)
    tys = np.array(tys)
    trans_txs = (txs*keyboard_width/DEFAULT_KEYBOARD_WIDTH) + dx
    trans_tys = (tys*keyboard_height/DEFAULT_KEYBOARD_HEIGHT) + dy
    return trans_txs, trans_tys

def generateLengthWiseSamplePoints(points_X, points_Y, gap):
    diff_x = np.diff(points_X)
    diff_y = np.diff(points_Y)
    length = np.sum(np.hypot(diff_x, diff_y))
    dots_num = int(length/gap) + 1
    
    return generateSamplePoints(points_X, points_Y, sample_num = dots_num)

def generateSamplePoints(points_X, points_Y, sample_num = 300):
    '''Generate 100 sampled points for a gesture.

    In this function, we should convert every gesture or template to a set of 100 points, such that we can compare
    the input gesture and a template computationally.

    :param points_X: A list of X-axis values of a gesture.
    :param points_Y: A list of Y-axis values of a gesture.

    :return:
        sample_points_X: A list of X-axis values of a gesture after sampling, containing 100 elements.
        sample_points_Y: A list of Y-axis values of a gesture after sampling, containing 100 elements.
    '''
    sample_points_X, sample_points_Y = [], []

    # converting [[x,x,x,x,x...]] to [x,x,x,x,x,...]
    if len(points_X) == 1:
        sample_points_X = points_X * sample_num
        sample_points_Y = points_Y * sample_num

    else:
        # calcuating cumulative distance of entire gesture using numpy library
        distance = np.cumsum(np.sqrt(np.ediff1d(points_X, to_begin=0) ** 2 + np.ediff1d(points_Y, to_begin=0) ** 2))
        distance = distance / distance[-1]
        # interpolating the entire line into 100 equi distance points
        try:
            fx, fy = interp1d(distance, points_X), interp1d(distance, points_Y)
            bet = np.linspace(0, 1, sample_num)
            # converting the numpy ndarrys to list
            sample_points_X = fx(bet).tolist()
            sample_points_Y = fy(bet).tolist()
        except:
            print('exception occured while interp1d')

    return sample_points_X, sample_points_Y

# normalize the gesture according to formula s = L/max(W,H)
def Normalize(X, Y, L = 100):
    minX = min(X)
    maxX = max(X)
    minY = min(Y)
    maxY = max(Y)

    # calculating the height and width of bounding box
    gesW = abs(maxX - minX)
    gesH = abs(maxY - minY)

    M = max(gesW, gesH)
    s = 1

    # M will be 0 for words like 'mm'
    nx = np.array(X)
    ny = np.array(Y)
    if M !=0:
        s = (L - 1) / M
        nx = nx*s
        ny = ny*s

    #shift coordinates 
    minX = minX * s
    maxX = maxX * s
    minY = minY * s
    maxY = maxY * s
        
     # shifting all the coordinates of the gesture
    sx = nx - minX
    sy = ny - minY
    return sx, sy

def squareNormalize(X, Y, L = 100, tuning = 8):
    minX = min(X)
    maxX = max(X)
    minY = min(Y)
    maxY = max(Y)

    # calculating the height and width of bounding box
    gesW = abs(maxX - minX)+1
    gesH = abs(maxY - minY)+1

    M = max(gesW, gesH)
    

    # M will be 0 for words like 'mm'
    nx = np.array(X)
    ny = np.array(Y)
    
    sig = np.tanh((gesH/gesW)*tuning)
    #sig = 1
    #print("square sigma" + str(sig))
    if M !=0:
        w_ratio = (L - 1)/gesW
        h_ratio = (L - 1)/(sig*gesH + (1-sig)*gesW)
        nx = nx*w_ratio
        ny = ny*h_ratio

    #shift coordinates 
    minX = minX * w_ratio
    minY = minY * h_ratio
        
     # shifting all the coordinates of the gesture
    sx = nx - minX
    sy = ny - minY
    return sx, sy

def getNearPointsIndices(sampled_xs,sampled_ys,xs,ys):
    diff_x = np.subtract.outer(sampled_xs, xs)
    diff_y = np.subtract.outer(sampled_ys, ys)
    indice = np.argmin(diff_x**2 + diff_y**2, axis = 1)
    return indice

def getLocationScore(gesture_sample_points_X, gesture_sample_points_Y, valid_template_sample_points_X,
                        valid_template_sample_points_Y):
     
    '''Get the location score for every valid word after pruning.

    In this function, we should compare the sampled user gesture (containing 100 points) with every single valid
    template (containing 100 points) and give each of them a location score.

    :param gesture_sample_points_X: A list of X-axis values of input gesture points, which has 100 values since we
        have sampled 100 points.
    :param gesture_sample_points_Y: A list of Y-axis values of input gesture points, which has 100 values since we
        have sampled 100 points.
    :param template_sample_points_X: 2D list, containing X-axis values of every valid template. Each of the
        elements is a 1D list and has the length of 100.
    :param template_sample_points_Y: 2D list, containing Y-axis values of every valid template. Each of the
        elements is a 1D list and has the length of 100.

    :return:
        A list of location scores.
    '''
    location_scores = []
    #locscore = locscore + (alpha(i)*delta(i,U,T,bigDval1,bigDval2))

    # calculating the shape score according to the formula
    ltempX = np.array(valid_template_sample_points_X)
    ltempY = np.array(valid_template_sample_points_Y)
    lgesX = np.array(gesture_sample_points_X)
    lgesY = np.array(gesture_sample_points_Y)
    
    diffsX= lgesX - ltempX
    diffsY = lgesY - ltempY
    location_scores = np.average(np.sqrt(diffsX*diffsX + diffsY*diffsY), axis = 1)

    
    return location_scores
def getShapeScores(gesture_normal_points_X, gesture_normal_points_Y, norm_valid_template_sample_points_X,
                     norm_valid_template_sample_points_Y, weights):
    '''Get the shape score for every valid word after pruning.

    In this function, we should compare the sampled input gesture (containing 100 points)
    with every single valid template (containing 100 points) 
    and give each of them a shape score.

    :param gesture_normal_points_X: A list of normalized X-axis values of input
    gesture points, which has 300 values since we
        have sampled 300 points.
    :param gesture_normal_points_Y: A list of normalized Y-axis values of input 
    gesture points, which has 300 values since we
        have sampled 300 points.
    :param valid_template_sample_points_X: 2D list, containing X-axis values of 
    every normalized valid template. Each of the
        elements is a 1D list and has the length of 300.
    :param valid_template_sample_points_Y: 2D list, containing Y-axis values of 
    every normalized valid template. Each of the
        elements is a 1D list and has the length of 300.

    :return:
        A list of shape scores.
    '''
    
    diffsX= gesture_normal_points_X - norm_valid_template_sample_points_X
    diffsY = gesture_normal_points_Y - norm_valid_template_sample_points_Y
    distances = np.sqrt(diffsX*diffsX + diffsY*diffsY)
    
    weights_distance = distances * weights

    shape_scores = np.average(weights_distance, axis = 1)*(1/np.mean(weights))

    return shape_scores