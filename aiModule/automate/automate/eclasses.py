import torch
from automate_cpp import find_equivalence_classes
import numpy as np
import pandas as pd

def find_eclasses(items, tolerance):
	if len(items) > 100000:
		return find_eclasses_multistage(items, tolerance)
	if type(items) == np.ndarray:
		items = [np.array(item) for item in items.tolist()]
	return find_equivalence_classes(items, tolerance)

def find_eclasses_multistage(items, tolerance):
	data = pd.Series([x.tobytes() for x in items])
	if type(items) != np.ndarray:
		items = np.array(items)
	unique_indices = (~data.duplicated())
	unique_items = items[unique_indices].tolist()
	
	unique_class = find_equivalence_classes(unique_items, tolerance)
	orig_indices = np.arrange(len(items))[unique_indices].to_list()
	unique_class_orig = [orig_indices[x] for x in unique_class]
	unique_data = data[unique_indices].to_list()

	class_map = dict(zip(unique_data, unique_class_orig))

	return [class_map[x] for x in data]

