#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jul 21 17:22:25 2020

@author: cuiwenzhe
"""
import numpy as np
import dill

def gaussian(x, mu, sig):
    return np.exp(np.power((x - mu)/sig, 2.)/-2.0) /sig * 2.50663 #2.50663 is sq(2*PI)

LANGUAGE_UNIGRAM_SIGMA = 7
LANGUAGE_BIGRAM_SIGMA = 0.07

vocab_words = []
vocab_freq = []
freqFile = open('freq.txt')
freqs = freqFile.readlines()

for line in freqs:
    w = line.strip().split('\t')
    vocab_words.append(w[0])
    vocab_freq.append(int(w[1]))

uniGramVocabProbs = dict(zip(vocab_words, vocab_freq))
leastUniGramProb = vocab_freq[-1]

vocab_probs_log = np.log(np.array(vocab_freq)/np.sum(vocab_freq))
gauss_vocab_freq = gaussian(vocab_probs_log, 0, LANGUAGE_UNIGRAM_SIGMA)

uniGramVocabGaussProbs = dict(zip(vocab_words, gauss_vocab_freq))
leastUniGramGaussProb = gauss_vocab_freq[-1]

def getUnigramLanguangeProbs(valid_words):
    probs = []
    for w in valid_words:
        if w in uniGramVocabProbs.keys():
            probs.append(uniGramVocabProbs[w])
        else:
            probs.append(leastUniGramProb)
    return np.array(probs/np.sum(probs))

def getUnigramLanguangeScores(valid_words):
    probs = []
    for w in valid_words:
        if w in uniGramVocabProbs.keys():
            probs.append(uniGramVocabGaussProbs[w])
        else:
            probs.append(leastUniGramGaussProb)
    return np.array(probs/np.sum(probs))
#%%
from collections import defaultdict
bigramModel = defaultdict(lambda: defaultdict(lambda: 0))
with open('sortedCOCABigram.dict', 'rb') as bigramModelFile:
    bigramModel = dill.load(bigramModelFile)
    
def getBigramLanguangeProbs(prev_word,valid_words):
    probs = []
    lowest_value = 1
    if len(bigramModel[prev_word]) > 10:
        lowest_key = list(bigramModel[prev_word])[-1]
        lowest_value = bigramModel[prev_word][lowest_key]
        
        for w in valid_words:
           raw_prob = bigramModel[prev_word][w]
           if raw_prob == 0:
               bigramModel[prev_word][w] = lowest_value
               raw_prob = lowest_value
           probs.append(raw_prob)
        probs = np.array(probs)
    if len(probs) == 0:
        probs = np.ones(len(valid_words))
        
    return probs


#%%
'''
GPT2 Language Model
'''
import torch
import numpy as np
from transformers import OpenAIGPTTokenizer, OpenAIGPTLMHeadModel
tokenizer = OpenAIGPTTokenizer.from_pretrained('openai-gpt')
model = OpenAIGPTLMHeadModel.from_pretrained('openai-gpt')


def getWordIds(words):
    return np.array(tokenizer(list(words))['input_ids'])
    
def getGPT2PredictionWordScores(pre_text, word_ids):
    inputs = tokenizer(pre_text, return_tensors="pt")
    outputs = model(**inputs, labels=inputs["input_ids"])
    loss, logits = outputs[:2]
    scores = logits[0][-1]
    scores = scores.detach().numpy()
    word_scores = []
    for i in word_ids:
        word_score = np.max(scores[i])
        word_scores.append(word_score)
    
    res = np.array(word_scores) - np.min(word_scores)
    #res = np.exp(word_scores)/sum(np.exp(word_scores))
    return res