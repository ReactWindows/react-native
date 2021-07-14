const {createSnapshotResolver} = require('@rnw-scripts/jest-out-of-tree-snapshot-resolver');
module.exports = createSnapshotResolver(__dirname);
