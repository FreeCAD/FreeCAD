import seaborn as sn
import numpy as np
from matplotlib.figure import Figure

def plot_confusion_matrix(cm, labels, normalization = (1, 'Actual Avg.'), name='', ax=None):
    norm_axis = normalization[0]
    cm=cm.cpu().numpy()
    if norm_axis >= 0:
        totals = cm.sum(norm_axis, keepdims=True)
        totals[np.where(totals == 0)] = 1
        cm = cm / totals
    fmt = '.1%'
    if norm_axis < 0:
        fmt = 'n'
    
    fig = Figure(figsize=(8, 8))
    ax = fig.add_subplot()
    ax = sn.heatmap(cm, annot=True, fmt=fmt, cmap='PuBu', ax=ax)
    name = name + ' ' if len(name) > 0 else name
    ax.set_title(f'{name}Confusion Matrix: {normalization[1]}')
    ax.set_ylabel('Actual')
    ax.set_xlabel('Predicted')
    ax.set_xticklabels(labels, rotation=45)
    ax.set_yticklabels(labels, rotation=0)
    return fig
