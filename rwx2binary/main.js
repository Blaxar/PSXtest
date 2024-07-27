import {Triangle, Vector3} from 'three';
import RWXLoader from 'three-rwx-loader';

const rwxPath = process.env.PSX_RWX_PATH;
const rwxProp = process.env.PSX_RWX_PROP;

const loader = new RWXLoader();
loader.setPath(`${rwxPath}/rwx`)
    .setResourcePath(`${rwxPath}/textures`)
    .setFlatten(true).setEnableTextures(false);

const loadRWX = (rwx) => {
  const positionArray = rwx.geometry.getAttribute('position').array;
  const indexArray = rwx.geometry.index.array;
  const positionData = new Uint8Array(positionArray.buffer);
  const indexData = new Uint8Array(indexArray.buffer);
  const normals = new Float32Array(indexArray.length);
  const normal = new Vector3();

  for (let i = 0; i < indexArray.length / 3; i++) {
    
    const triangle = new Triangle(
      new Vector3(positionArray[indexArray[i * 3] * 3],
                  positionArray[indexArray[i * 3] * 3 + 1],
                  positionArray[indexArray[i * 3] * 3 + 2]),
      new Vector3(positionArray[indexArray[i * 3 + 1] * 3],
                  positionArray[indexArray[i * 3 + 1] * 3 + 1],
                  positionArray[indexArray[i * 3 + 1] * 3 + 2]),
      new Vector3(positionArray[indexArray[i * 3 + 2] * 3],
                  positionArray[indexArray[i * 3 + 2] * 3 + 1],
                  positionArray[indexArray[i * 3 + 2] * 3 + 2])
    );

    triangle.getNormal(normal);
    normals[i * 3] = normal.x;
    normals[i * 3 + 1] = normal.y;
    normals[i * 3 + 2] = normal.z;
  }

  const normalData = new Uint8Array(normals.buffer);

  // We store the amount of vertices and the amount of faces at the begening of the array,
  // as unsigned 16-bit integers each, hence the additional 4 bytes
  const data = new Uint8Array(4 + positionData.length + indexData.length + normalData.length);
  const header = new Uint16Array(data.buffer);
  header[0] = positionArray.length / 3;
  header[1] = indexArray.length / 3;
  data.set(positionData, 4);
  data.set(indexData, 4 + positionData.length);
  data.set(normalData, 4 + positionData.length + indexData.length);

  console.log(Buffer.from(data.buffer).toString('base64'));
  window.close();
};

loader.load(rwxProp, loadRWX, null,
            (err) => {
              console.log(err);
              window.close();
            }
           );
