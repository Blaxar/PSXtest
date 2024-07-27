import * as readline from 'readline';
import {writeFile} from 'fs';

const decimalLength = 12;
const ONE = 1 << decimalLength;

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

const output = process.env.PSX_RWX_OUTPUT || './rwx.bin';

const toFixedPoint = (f, rightShift = 0) => {
  const decimal = Math.floor((f % 1) * ONE);
  // The decimal part will already carry the highest-power bit for the sign,
  // thus if we're dealing with a negative number: we need to adjust the integer
  // part or the final sum will overshoot -1 too far
  const integer = ((Math.floor(f) + ((decimal < 0) ? 1 : 0)) << decimalLength);
  return (integer + decimal) >> rightShift;
};

rl.on('line', (line) => {
  const inputData = new Uint8Array(Buffer.from(line, 'base64'));
  const header = new Uint16Array(inputData.buffer);
  const nbVertices = header[0];
  const nbFaces = header[1];

  // Perform integrity check
  const dataLength = (4 + nbVertices * 12 + nbFaces * (18));
  if (inputData.length != dataLength) {
    console.log(`Unexpected raw data length: ${inputData.length} (expected: ${dataLength})`);
    process.exit(1);
  };

  // Convert each f32 value to fixed point 16-bit integer (12-bit decimal)
  const floatVertices =
      new Float32Array(inputData.slice(4, 4 + nbVertices * 12).buffer);
  const fixedVertices = new Int16Array(nbVertices * 4); // Using padding to match PSX data layout
  const faces =
      (new Int16Array(inputData.buffer)).slice(2 + nbVertices * 6, 2 + nbVertices * 6 + nbFaces * 3);
  const floatNormals =
      new Float32Array(inputData.slice(4 + nbVertices * 12 + nbFaces * 6).buffer);
  const fixedNormals = new Int16Array(nbFaces * 4); // Using padding to match PSX data layout

  if (floatVertices.length != nbVertices * 3 ||
    fixedVertices.length != nbVertices * 4) {
    console.log(`Unexpected vertex data length: ${floatVertices.length} and ${fixedVertices.length} (expected: ${nbVertices * 3} and ${nbVertices * 4})`);
    process.exit(1);
  }

  if (faces.length != nbFaces * 3) {
    console.log(`Unexpected face data length: ${faces.length} (expected: ${nbFaces * 3})`);
    process.exit(1);
  }

  if (floatNormals.length != nbFaces * 3 ||
    fixedNormals.length != nbFaces * 4) {
    console.log(`Unexpected normal data length: ${floatNormals.length} and ${fixedNormals.length} (expected: ${nbFaces * 3} and ${nbFaces * 4})`);
    process.exit(1);
  }

  for (let id = 0; id < nbVertices; id++) {
    fixedVertices[id * 4] = toFixedPoint(floatVertices[id * 3]);
    fixedVertices[id * 4 + 1] = -toFixedPoint(floatVertices[id * 3 + 1]);
    fixedVertices[id * 4 + 2] = toFixedPoint(floatVertices[id * 3 + 2]);
    fixedVertices[id * 4 + 3] = 0;
  }

  for (let id = 0; id < nbFaces; id++) {
    fixedNormals[id * 4] = toFixedPoint(floatNormals[id * 3]);
    fixedNormals[id * 4 + 1] = -toFixedPoint(floatNormals[id * 3 + 1]);
    fixedNormals[id * 4 + 2] = toFixedPoint(floatNormals[id * 3 + 2]);
    fixedNormals[id * 4 + 3] = 0;
  }

  const outputData = new Uint8Array(4 + (nbVertices + nbFaces) * 8 + nbFaces * 6);
  const outputHeader = new Uint16Array(outputData.buffer);
  outputHeader[0] = nbVertices;
  outputHeader[1] = nbFaces;
  outputData.set(new Uint8Array(fixedVertices.buffer), 4);
  outputData.set(new Uint8Array(faces.buffer), 4 + nbVertices * 8);
  outputData.set(new Uint8Array(fixedNormals.buffer), 4 + nbVertices * 8 + nbFaces * 6);

  writeFile(
    output,
    outputData,
    (err) => {
      if (err) throw err;
      console.log(`The file has been saved to '${output}'!`);
    });
});
