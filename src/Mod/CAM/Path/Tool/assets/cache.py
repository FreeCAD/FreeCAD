# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import time
import hashlib
import logging
from collections import OrderedDict
from typing import Any, Dict, Set, NamedTuple, Optional, Tuple

# For type hinting Asset and AssetUri to avoid circular imports
# from typing import TYPE_CHECKING
# if TYPE_CHECKING:
#     from .asset import Asset
#     from .uri import AssetUri

logger = logging.getLogger(__name__)


class CacheKey(NamedTuple):
    store_name: str
    asset_uri_str: str
    raw_data_hash: int
    dependency_signature: Tuple


class CachedAssetEntry(NamedTuple):
    asset: Any  # Actually type Asset
    size_bytes: int  # Estimated size of the raw_data
    timestamp: float  # For LRU, or just use OrderedDict nature


class AssetCache:
    def __init__(self, max_size_bytes: int = 100 * 1024 * 1024):  # Default 100MB
        self.max_size_bytes: int = max_size_bytes
        self.current_size_bytes: int = 0

        self._cache: Dict[CacheKey, CachedAssetEntry] = {}
        self._lru_order: OrderedDict[CacheKey, None] = OrderedDict()

        self._cache_dependents_map: Dict[str, Set[CacheKey]] = {}
        self._cache_dependencies_map: Dict[CacheKey, Set[str]] = {}

    def _evict_lru(self):
        while self.current_size_bytes > self.max_size_bytes and self._lru_order:
            oldest_key, _ = self._lru_order.popitem(last=False)
            if oldest_key in self._cache:
                evicted_entry = self._cache.pop(oldest_key)
                self.current_size_bytes -= evicted_entry.size_bytes
                logger.debug(
                    f"Cache Evict (LRU): {oldest_key}, "
                    f"size {evicted_entry.size_bytes}. "
                    f"New size: {self.current_size_bytes}"
                )
                self._remove_key_from_dependency_maps(oldest_key)

    def _remove_key_from_dependency_maps(self, cache_key_to_remove: CacheKey):
        direct_deps_of_removed = self._cache_dependencies_map.pop(cache_key_to_remove, set())
        for dep_uri_str in direct_deps_of_removed:
            if dep_uri_str in self._cache_dependents_map:
                self._cache_dependents_map[dep_uri_str].discard(cache_key_to_remove)
                if not self._cache_dependents_map[dep_uri_str]:
                    del self._cache_dependents_map[dep_uri_str]

    def get(self, key: CacheKey) -> Optional[Any]:
        if key in self._cache:
            self._lru_order.move_to_end(key)
            logger.debug(f"Cache HIT: {key}")
            return self._cache[key].asset
        logger.debug(f"Cache MISS: {key}")
        return None

    def put(
        self,
        key: CacheKey,
        asset: Any,
        raw_data_size_bytes: int,
        direct_dependency_uri_strs: Set[str],
    ):
        if key in self._cache:
            self._remove_key_from_dependency_maps(key)
            self.current_size_bytes -= self._cache[key].size_bytes
            del self._cache[key]
            self._lru_order.pop(key, None)

        if raw_data_size_bytes > self.max_size_bytes:
            logger.warning(
                f"Asset {key.asset_uri_str} (size {raw_data_size_bytes}) "
                f"too large for cache (max {self.max_size_bytes}). Not caching."
            )
            return

        self.current_size_bytes += raw_data_size_bytes
        entry = CachedAssetEntry(asset=asset, size_bytes=raw_data_size_bytes, timestamp=time.time())
        self._cache[key] = entry
        self._lru_order[key] = None
        self._lru_order.move_to_end(key)

        self._cache_dependencies_map[key] = direct_dependency_uri_strs
        for dep_uri_str in direct_dependency_uri_strs:
            self._cache_dependents_map.setdefault(dep_uri_str, set()).add(key)

        logger.debug(
            f"Cache PUT: {key}, size {raw_data_size_bytes}. "
            f"Total cache size: {self.current_size_bytes}"
        )
        self._evict_lru()

    def invalidate_for_uri(self, updated_asset_uri_str: str):
        keys_to_remove_from_cache: Set[CacheKey] = set()
        invalidation_queue: list[str] = [updated_asset_uri_str]
        processed_uris_for_invalidation_round: Set[str] = set()

        while invalidation_queue:
            current_uri_to_check_str = invalidation_queue.pop(0)
            if current_uri_to_check_str in processed_uris_for_invalidation_round:
                continue
            processed_uris_for_invalidation_round.add(current_uri_to_check_str)

            for ck in list(self._cache.keys()):
                if ck.asset_uri_str == current_uri_to_check_str:
                    keys_to_remove_from_cache.add(ck)

            dependent_cache_keys = self._cache_dependents_map.get(
                current_uri_to_check_str, set()
            ).copy()

            for dep_ck in dependent_cache_keys:
                if dep_ck not in keys_to_remove_from_cache:
                    keys_to_remove_from_cache.add(dep_ck)
                    parent_uri_of_dep_ck = dep_ck.asset_uri_str
                    if parent_uri_of_dep_ck not in processed_uris_for_invalidation_round:
                        invalidation_queue.append(parent_uri_of_dep_ck)

        for ck_to_remove in keys_to_remove_from_cache:
            if ck_to_remove in self._cache:
                entry_to_remove = self._cache.pop(ck_to_remove)
                self.current_size_bytes -= entry_to_remove.size_bytes
                self._lru_order.pop(ck_to_remove, None)
                self._remove_key_from_dependency_maps(ck_to_remove)

        if keys_to_remove_from_cache:
            logger.debug(
                f"Cache invalidated for URI '{updated_asset_uri_str}' and "
                f"its dependents. Removed {len(keys_to_remove_from_cache)} "
                f"entries. New size: {self.current_size_bytes}"
            )

    def clear(self):
        self._cache.clear()
        self._lru_order.clear()
        self._cache_dependents_map.clear()
        self._cache_dependencies_map.clear()
        self.current_size_bytes = 0
        logger.info("AssetCache cleared.")
