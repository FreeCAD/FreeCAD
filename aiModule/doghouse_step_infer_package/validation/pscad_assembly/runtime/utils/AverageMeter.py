
class AverageMeter(object):
    def __init__(self, items=None):
        self.items = items
        self.n_items = 1 if items is None else len(items)
        self.reset()

    def reset(self):
        self._val = [0] * self.n_items
        self._sum = [0] * self.n_items
        self._count = [0] * self.n_items

    def update(self, values):
        if type(values).__name__ == 'list':
            for idx, v in enumerate(values):
                self._val[idx] = v
                self._sum[idx] += v
                self._count[idx] += 1
        else:
            self._val[0] = values
            self._sum[0] += values
            self._count[0] += 1

    def val(self, idx=None):
        if idx is None:
            return self._val[0] if self.items is None else [self._val[i] for i in range(self.n_items)]
        else:
            return self._val[idx]

    def count(self, idx=None):
        if idx is None:
            return self._count[0] if self.items is None else [self._count[i] for i in range(self.n_items)]
        else:
            return self._count[idx]

    def avg(self, idx=None):
        if idx is None:
            return self._sum[0] / self._count[0] if self.items is None else [
                self._sum[i] / self._count[i] for i in range(self.n_items)
            ]
        else:
            return self._sum[idx] / self._count[idx]