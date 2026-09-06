/*
 * search-inventory.js: hand-written registry of every search/filter field
 * on the site. tests.html cross-checks this list against the live DOM: any
 * inventory row missing its builder, or any field with a builder missing
 * from this list, is reported as a visible failure.
 */
(function (global) {
  'use strict';
  global.SEARCH_INVENTORY = [
    { id: 'palette-search', label: 'Command palette search', selector: '#palette-input' },
    { id: 'docs-search', label: 'Documentation browser search', selector: '#docs-search-input' },
    { id: 'changelog-filter', label: 'Changelog filter', selector: '#changelog-filter-input' },
    { id: 'gallery-filter', label: 'Gallery filter', selector: '#gallery-filter-input' },
    { id: 'features-filter', label: 'Features filter', selector: '#features-filter-input' },
    { id: 'downloads-filter', label: 'Downloads filter', selector: '#downloads-filter-input' },
    { id: 'settings-search', label: 'Settings search', selector: '#settings-search-input' },
    { id: 'tab-search', label: 'Documentation tab search', selector: '#tab-search-input' },
    { id: 'history-search', label: 'Settings history search', selector: '#history-search-input' },
  ];
})(window);
