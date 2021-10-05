#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Aug  6 23:34:25 2020

@author: cuiwenzhe
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Aug  3 17:06:50 2020

@author: cuiwenzhe
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Jul 31 12:41:57 2020

@author: cuiwenzhe
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jul 29 16:59:10 2020

@author: cuiwenzhe
"""
import os
import pickle
import numpy as np
from tqdm import tqdm
from scipy.interpolate import interp1d
import KeyboardUtil
from LanguageUtil import getUnigramLanguangeProbs, getGPT2PredictionWordScores,getBigramLanguangeProbs, getWordIds, gaussian, vocab_words

rootdir = os.getcwd()
file_path = os.path.join(rootdir, "./decode_setup_20011.pkl")

def getWordLenth(word):
    count = 0
    cur = 0
    for c in word:
        if c != cur:
            count += 1
        cur = c
    return count

with open(file_path,'rb') as file:
    dict_words, dict_words_length, dict_words_corners, normal_templates_xs, normal_templates_ys, sample_templates_xs, sample_templates_ys, all_word_ids = pickle.load(file)

command_path = os.path.join(rootdir, "./backscreen_commands.txt")
command_raw_words = set()
with open(command_path) as file:
    lines = file.readlines()
    for w in lines:
        if len(w) != 0:
            command_raw_words.add(w.strip())  

command_words = []
command_words_length = []
command_words_corners = []
cmd_normal_templates_xs = []
cmd_normal_templates_ys = []
cmd_sample_templates_xs = []
cmd_sample_templates_ys = []

for word in tqdm(command_raw_words):
    word = word.strip()
    if not word.isalpha:
        continue
    cmd_raw_xs = []
    cmd_raw_ys = []
    for c in word:
        cmd_raw_xs.append(KeyboardUtil.centroids_X[ord(c)-97])
        cmd_raw_ys.append(KeyboardUtil.centroids_Y[ord(c)-97])
    
    cmd_sample_xs, cmd_sample_ys = KeyboardUtil.generateSamplePoints(cmd_raw_xs, cmd_raw_ys)

    #normal_xs, normal_ys = KeyboardUtil.squareNormalize(sample_xs, sample_ys, tuning = 6)
    cmd_normal_xs, cmd_normal_ys = KeyboardUtil.Normalize(cmd_sample_xs, cmd_sample_ys)

    command_words.append(word)
    command_words_length.append(getWordLenth(word))
    command_words_corners.append(KeyboardUtil.getSharpWordCorner(word, 90))
    cmd_normal_templates_xs.append(cmd_normal_xs)
    cmd_normal_templates_ys.append(cmd_normal_ys) 
    cmd_sample_templates_xs.append(cmd_sample_xs)
    cmd_sample_templates_ys.append(cmd_sample_ys)

cmd_normal_templates_xs = np.array(cmd_normal_templates_xs)
cmd_normal_templates_ys = np.array(cmd_normal_templates_ys)
cmd_sample_templates_xs = np.array(cmd_sample_templates_xs)
cmd_sample_templates_ys = np.array(cmd_sample_templates_ys)

command_words = np.array(command_words)
command_words_length = np.array(command_words_length)
command_words_corners = np.array(command_words_corners)

cmd_word_ids = getWordIds(command_words)

#%%
def noPruning():
    valid_words = dict_words
    valid_indices = np.array(range(len(dict_words)))
    norm_valid_template_sample_points_X = normal_templates_xs
    norm_valid_template_sample_points_Y = normal_templates_ys
    valid_template_sample_points_X = sample_templates_xs
    valid_template_sample_points_Y = sample_templates_ys
    
    return valid_words, valid_indices, norm_valid_template_sample_points_X, norm_valid_template_sample_points_Y, valid_template_sample_points_X, valid_template_sample_points_Y

def commandNoPruning():
    cmd_valid_words = command_words
    cmd_valid_indices = np.array(range(len(command_words)))
    cmd_norm_valid_template_sample_points_X = cmd_normal_templates_xs
    cmd_norm_valid_template_sample_points_Y = cmd_normal_templates_ys
    cmd_valid_template_sample_points_X = cmd_sample_templates_xs
    cmd_valid_template_sample_points_Y = cmd_sample_templates_ys
    
    return cmd_valid_words, cmd_valid_indices, cmd_norm_valid_template_sample_points_X, \
        cmd_norm_valid_template_sample_points_Y, cmd_valid_template_sample_points_X, cmd_valid_template_sample_points_Y
 

def cornerPruning(pred_numbers):
    valid_indices = (dict_words_corners == -1)
    for num in pred_numbers:
        valid_indices = np.logical_or(valid_indices, dict_words_corners == num)
   
    valid_words = dict_words[valid_indices]
    norm_valid_template_sample_points_X = normal_templates_xs[valid_indices]
    norm_valid_template_sample_points_Y = normal_templates_ys[valid_indices]
    valid_template_sample_points_X = sample_templates_xs[valid_indices]
    valid_template_sample_points_Y = sample_templates_ys[valid_indices]
    return valid_words, valid_indices, norm_valid_template_sample_points_X, norm_valid_template_sample_points_Y, valid_template_sample_points_X, valid_template_sample_points_Y

def commandCornerPruning(pred_numbers):
    cmd_valid_indices = (command_words_corners == -1)
    for num in pred_numbers:
        cmd_valid_indices = np.logical_or(cmd_valid_indices, command_words_corners == num)
   
    cmd_valid_words = command_words[cmd_valid_indices]
    cmd_norm_valid_template_sample_points_X = cmd_normal_templates_xs[cmd_valid_indices]
    cmd_norm_valid_template_sample_points_Y = cmd_normal_templates_ys[cmd_valid_indices]
    cmd_valid_template_sample_points_X = cmd_sample_templates_xs[cmd_valid_indices]
    cmd_valid_template_sample_points_Y = cmd_sample_templates_ys[cmd_valid_indices]
    if len(cmd_valid_words) == 0:
        print("Command No Pruning")
        return commandNoPruning()
    else:
        return cmd_valid_words, cmd_valid_indices, cmd_norm_valid_template_sample_points_X, \
            cmd_norm_valid_template_sample_points_Y, cmd_valid_template_sample_points_X, cmd_valid_template_sample_points_Y        
def getBestWord(valid_words, integration_scores, language_scores, candidate_num):
    '''Get the best word.

    In this function, you should select top-n words with the highest integration 
    scores and then use their corresponding probability 
    (stored in variable "probabilities") as weight. 
    The word with the highest weighted integration score is
    exactly the word we want.

    :param valid_words: A list of valid words.
    :param integration_scores: A list of corresponding integration scores of valid_words.
    :return: The most probable word suggested to the user.
    '''
    best_words = []
    best_scores = []
    # TODO: Set your own range.
    n = candidate_num
    
    #sort start from big value
    combine_scores = integration_scores*language_scores
    
    sorted_arg = np.argsort(-combine_scores)
    
    valid_indices = sorted_arg[:n]
    decode_scores = np.array(combine_scores)[valid_indices]
    decode_words = np.array(valid_words)[valid_indices]
    if len(decode_words) > 0:
        best_words = list(decode_words)
        best_scores = list(decode_scores)
            
    return best_words, best_scores

from KeyboardUtil import generateSamplePoints, Normalize, squareNormalize, getNearPointsIndices, generateLengthWiseSamplePoints, \
        getShapeScores, getGestureKeyCenters, getDxDy, getKeyboardOriMean, transformSampledTemplate, getLocationScore
from FeatureUtil import getArcTans, getSpeeds, getDensity, getCorners, getDegrees
from FigureUtil import getEnvelope, drawGrayDots, drawWaveAndLine

MEAN_GESTURE_KEYBOARD_WIDTH = 381
MEAN_GESTURE_KEYBOARD_HEIGHT = 318
def decodingGesture(xs, ys, dx, dy, prev_word, prev_text, weights_sigma = 0.006, squareTuning = 6):
    
    sx, sy = generateSamplePoints(xs, ys)
    nsx, nsy = generateLengthWiseSamplePoints(xs, ys, 30)
    nx, ny = Normalize(sx, sy)
    
    ref_indices = getNearPointsIndices(sx, sy, xs, ys)
    if len(xs) != 1:
        '''
        Desity feature
        '''
        density = getDensity(xs, ys)
        density = density/np.sum(density)
        gauss_dense = gaussian(density, 0, weights_sigma)
    
        '''
        curve feature
        '''
        
        arctans = getDegrees(nsx,nsy)
        print()
        arctan_gradient = np.gradient(arctans)
        valid_corners = getCorners(arctan_gradient)
        
        
        weights = gauss_dense[ref_indices]     
        

    else:
        weights = 1
        valid_corners = [-1]
    if prev_word == '_HEAD_':
        print('SEGMENT_PRUNING:' + str(valid_corners) )
        valid_words, valid_indices, norm_valid_template_sample_points_X, norm_valid_template_sample_points_Y, valid_template_sample_points_X, valid_template_sample_points_Y = \
            cornerPruning(valid_corners)
    else:
        #print('PEAK_PRUNING:' + str(valid_peaks) )
        valid_words, valid_indices, norm_valid_template_sample_points_X, norm_valid_template_sample_points_Y, valid_template_sample_points_X, valid_template_sample_points_Y = \
            cornerPruning(valid_corners)
            #numberPruningNew(valid_peaks)
        # segmentPruning(valid_segments)
    shape_scores = getShapeScores(nx, ny, \
                                    norm_valid_template_sample_points_X,norm_valid_template_sample_points_Y,weights)
    
    shape_gauss = gaussian(shape_scores, 0, 60) 
    shape_probs = shape_gauss/np.sum(shape_gauss)
     

    location_probs = 1
    if dx != 0 and dy != 0:
        ox = 190.5
        oy = 159.0
        meanx = np.mean(xs) - ox
        meany = np.mean(ys) - oy
        
        change = abs(np.hypot(dx,dy) - np.hypot(meanx, meany))
        
        if  change < 150: 
            tvx, tvy = transformSampledTemplate(valid_template_sample_points_X, valid_template_sample_points_Y, dx, dy, MEAN_GESTURE_KEYBOARD_WIDTH, MEAN_GESTURE_KEYBOARD_HEIGHT)
            location_scores = getShapeScores(sx, sy, tvx, tvy, weights)
            
            location_gauss = gaussian(location_scores/10, 0, 10) 
            location_probs = location_gauss/np.sum(location_gauss)
     
    integration_scores = np.power(shape_probs,1) * np.power(location_probs,0.1)
    
    valid_ids = all_word_ids[valid_indices]
    
    print(prev_text)
    if len(prev_text.strip().split(' ')) < 2:
        print('Bigram:')
        language_scores = getBigramLanguangeProbs(prev_word, valid_words)
        language_scores = np.power(language_scores,0.03)
    else:
        print('GPT2:')
        language_scores = getGPT2PredictionWordScores(prev_text,valid_ids)
        language_scores = np.power(language_scores,0.5)
        
    language_scores = language_scores/np.sum(language_scores)
    # print("prev_text|" + prev_text + "|")

    
    best_words, best_scores = getBestWord(valid_words, integration_scores, language_scores, 20)
    print(best_words)
    #print(best_scores)
    return best_words, best_scores

def decodingCommandGesture(xs, ys, dx, dy, prev_word, prev_text, weights_sigma = 0.006, squareTuning = 6):
    
    sx, sy = generateSamplePoints(xs, ys)
    nx, ny = Normalize(sx, sy)
    nsx, nsy = generateLengthWiseSamplePoints(xs, ys, 30)
    ref_indices = getNearPointsIndices(sx, sy, xs, ys)
    if len(xs) != 1:
        '''
        Desity feature
        '''
        density = getDensity(xs, ys)
        density = density/np.sum(density)
        gauss_dense = gaussian(density, 0, weights_sigma)
        
        weights = gauss_dense[ref_indices]     
        
        fit_grad, peak_num = getEnvelope(density,'peak') 
       
        fit_grad2, peak_num2 = getEnvelope(fit_grad,'peak')
     
        
        '''
        curve feature
        '''
        
        arctans = getDegrees(nsx, nsy)
        #cut_off
        arctans = arctans[int(len(arctans)/10):]
        arctan_gradient = np.gradient(arctans)
        valid_corners = getCorners(arctan_gradient)
    else:
        weights = 1
        valid_corners = [-1]
    
    valid_words, valid_indices, norm_valid_template_sample_points_X, norm_valid_template_sample_points_Y, valid_template_sample_points_X, valid_template_sample_points_Y = \
        commandCornerPruning(valid_corners)
        #commandNumberPruningNew(valid_peaks)

    shape_scores = getShapeScores(nx, ny, \
                                    norm_valid_template_sample_points_X,norm_valid_template_sample_points_Y,weights)
    
    max_shape_score = np.max(shape_scores)
    min_shape_score = np.min(shape_scores)
    if max_shape_score - min_shape_score != 0:
        shape_scores = (max_shape_score - shape_scores)/(max_shape_score - min_shape_score)
    shape_probs = shape_scores/np.sum(shape_scores)
    
 
    best_words, best_scores = getBestWord(valid_words, shape_probs, 1, 229)
    print(best_words)
    print(best_scores)
    return best_words, best_scores

#%%
import socket
import threading
import traceback
import struct
from socketserver import TCPServer, UDPServer, BaseRequestHandler, StreamRequestHandler
import json
DECODER_PORT = 10086

TCPServer.allow_reuse_address = True
UDPServer.allow_reuse_address = True
temp_xs = []
temp_ys = []
dx = 0
dy = 0
def get_ip_address():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]

class SniffRequestHandler(BaseRequestHandler):
    def handle(self):
        try:
            data = self.request[0].strip()          
            client_ip, port = self.client_address
            msg = data.decode("utf-8")
            message = 'Received from %s' % (msg)
            code, client_port = msg.split(':')
            
            print(message)
            if code == "RequestServerForGesture":
                sendSocket = socket.socket()
                print("send back to client:" + client_ip + ":" + client_port)
                try:
                    sendSocket.connect((client_ip,int(client_port)))
                    back_msg = str(get_ip_address())
                    back_msg += ":" + str(DECODER_PORT)
                    print(back_msg)
                    sendSocket.send(back_msg.encode())
                except: 
                    print("connection failed")
                finally:
                    sendSocket.close()
        except:
            traceback.print_exc()

sniff_address = ('', 9527)
sniff_server = UDPServer(sniff_address, SniffRequestHandler)

def startSniffServer():
    with sniff_server:
        sniff_server.serve_forever()       

def removeWords(words, scores, banned_words):
    targets = []
    targets_scores = []
    #print("removing ", banned_words)
    for i in range(len(words)):
        w = words[i]
        s = scores[i]
        if w not in banned_words:
            targets.append(w)
            targets_scores.append(s)
    return targets, targets_scores

def encodeResultJSON(result_words, result_scores):
    if len(result_words) == 0:
        print('Fail to decode gesture')
        return 
    result = {'RESULT_WORDS':result_words,
               'RESULT_SCORES':result_scores}
    return json.dumps(result).encode('utf8')
    
class GestureRequestHandler(StreamRequestHandler):
    def handle(self):
        '''
        The first number in the data is the number of the gesture points 
        The first half of the data is x axes of the gestures points.
        The other half of the data is y axes of the gestures points
        '''
        global temp_xs, temp_ys, dx, dy
        self.data = self.rfile.read()
        
        json_data = self.data.decode("utf-8")
        json_dict = json.loads(json_data)
        task = json_dict['TASK']
        if task == "DECODE":
            prev_word = json_dict['PREV_WORD']
            raw_undo_words = json_dict['UNDO_WORDS']
            prev_text = json_dict['PREV_TEXT']
            client_receive_port = json_dict['PORT']
            
            print("----PHRASE----")
            '''
            strip '[' ']' in Java list string using [1:-1]
            '''
            xpoints = json_dict['XPOINTS'][1:-1].split(',')
            ypoints = json_dict['YPOINTS'][1:-1].split(',')
            orientations = json_dict['ORIENTATIONS'][1:-1].split(',')
            timestamps = json_dict['TIMESTAMPS'][1:-1].split(',')
            velocities = json_dict['VELOCITIES'][1:-1].split(',')
            
            xpoints = [int(p) for p in xpoints]
            ypoints = [int(p) for p in ypoints]
            orientations = [float(p) for p in orientations]
            timestamps = [int(p) for p in timestamps]
            velocities = [float(p) for p in velocities]
            
            xs = xpoints
            ys = ypoints
            temp_xs = xpoints
            temp_ys = ypoints
            
            #print("prev_word|" + prev_word + "|")
            if len(prev_word) == 0:
                prev_word = "_HEAD_"
                prev_text = "_HEAD_"
            best_words, best_scores = decodingGesture(xs, ys, dx, dy, prev_word, prev_text)
            result_words, result_scores = removeWords(best_words, best_scores, raw_undo_words)
            result_bytes = encodeResultJSON(result_words,result_scores)
            
            #print("result_words is ", result_words)
            client_ip, port = self.client_address
            #print(client_ip, port)
            
            
            sendSocket = socket.socket()
            #print("send back to client:" + client_ip + ":" + client_receive_port)
            try:
                sendSocket.connect((client_ip,int(client_receive_port)))
                sendSocket.send(result_bytes)
            except: 
                print("connection failed")
            finally:
                sendSocket.close()
        elif task == "CMD_DECODE":
            prev_word = json_dict['PREV_WORD']
            raw_undo_words = json_dict['UNDO_WORDS']
            prev_text = json_dict['PREV_TEXT']
            client_receive_port = json_dict['PORT']
            
            print("----CMD----")
            '''
            strip '[' ']' in Java list string using [1:-1]
            '''
            xpoints = json_dict['XPOINTS'][1:-1].split(',')
            ypoints = json_dict['YPOINTS'][1:-1].split(',')
            orientations = json_dict['ORIENTATIONS'][1:-1].split(',')
            timestamps = json_dict['TIMESTAMPS'][1:-1].split(',')
            velocities = json_dict['VELOCITIES'][1:-1].split(',')
            
            xpoints = [int(p) for p in xpoints]
            ypoints = [int(p) for p in ypoints]
            orientations = [float(p) for p in orientations]
            timestamps = [int(p) for p in timestamps]
            velocities = [float(p) for p in velocities]
            
            xs = xpoints
            ys = ypoints
            temp_xs = xpoints
            temp_ys = ypoints
            
            #print("prev_word|" + prev_word + "|")
            if len(prev_word) == 0:
                prev_word = "_HEAD_"
                prev_text = "_HEAD_"
            best_words, best_scores = decodingCommandGesture(xs, ys, dx, dy, prev_word, prev_text)
            result_words, result_scores = removeWords(best_words, best_scores, raw_undo_words)
            result_bytes = encodeResultJSON(result_words,result_scores)
            #print("result_words is ", result_words)
            client_ip, port = self.client_address
            #print(client_ip, port)
            
            
            sendSocket = socket.socket()
            #print("send back to client:" + client_ip + ":" + client_receive_port)
            try:
                sendSocket.connect((client_ip,int(client_receive_port)))
                sendSocket.send(result_bytes)
            except: 
                print("connection failed")
            finally:
                sendSocket.close()
        elif task == "CONFIRM":
            confirmed_word = json_dict['CUR_WORD']
            sx, sy = generateSamplePoints(temp_xs, temp_ys)
            gesture_letter_xs, gesture_letter_ys = getGestureKeyCenters(confirmed_word,sx,sy)
            dx, dy = getDxDy(confirmed_word,gesture_letter_xs, gesture_letter_ys, 381, 318)
            #print("confirmed word is:" + confirmed_word)
            #print("(dx, dy) = " + str((dx, dy)))
            
        elif task == "UNDO":
            dx = 0
            dy = 0
            temp_xs = []
            temp_ys = []
            respond_undo_words = json_dict['CUR_WORD']
            #print("undo word is:",respond_undo_words)
            

gesture_adderss = ('', DECODER_PORT)
gesture_server = TCPServer(gesture_adderss, GestureRequestHandler)

def startGestureServer():
    with gesture_server:
        gesture_server.serve_forever()
    


#%%
import atexit 
  
@atexit.register 
def clean():
    global sniff_server
    global gesture_server
    print('Stop server') 
    sniff_server.shutdown()
    gesture_server.shutdown()
    sniff_server = None
    gesture_server = None
    

import sys
 
def startServer():
    print("Server is running")
    sniffingThread = threading.Thread(target=startSniffServer)
    sniffingThread.start()
    
    gestureThread = threading.Thread(target=startGestureServer)
    gestureThread.start()
    
if __name__ == "__main__":
    args = sys.argv
    startServer()
    
    

