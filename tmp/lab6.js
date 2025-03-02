"use strict";

var canvas;
var gl;

var numTimesToSubdivide = 0;

var index = 0;

var positionsArray = [];
var normalsArray = [];

// tetrahedron vertex positions
var va = vec4(0.0, 0.0, -1.0,1);
var vb = vec4(0.0, 0.942809, 0.333333, 1);
var vc = vec4(-0.816497, -0.471405, 0.333333, 1);
var vd = vec4(0.816497, -0.471405, 0.333333,1);

// viewing window
var near = -10;
var far = 10;
var radius = 1.5;
var theta = 0.0;
var phi = 0.0;
var dr = 5.0 * Math.PI/180.0;

var left = -3.0;
var right = 3.0;
var top0 =3.0;	// "top" conflicts with JS native variables
var bottom = -3.0;

// light position and parameters
var lightPosition = vec4(2.0, 2.0, 2.0, 1.0);
var lightAmbient = vec4(0.2, 0.2, 0.2, 1.0);
var lightDiffuse = vec4(1.0, 1.0, 1.0, 1.0);
var lightSpecular = vec4(1.0, 1.0, 1.0, 1.0);

// material parameters
var materialAmbient = vec4(1.0, 0.0, 1.0, 1.0);
var materialDiffuse = vec4(1.0, 0.8, 0.0, 1.0);
var materialSpecular = vec4(1.0, 1.0, 1.0, 1.0);
var materialShininess = 100.0;


var modelViewMatrix, projectionMatrix;
var modelViewMatrixLoc, projectionMatrixLoc;

// "normal" matrix
var nMatrix, nMatrixLoc;

var eye;
var at = vec3(0.0, 0.0, 0.0);
var up = vec3(0.0, 1.0, 0.0);

function triangle(a, b, c) {
// push triangle vertex positions to positionArray
     positionsArray.push(a);
     positionsArray.push(b);
     positionsArray.push(c);
     
    // compute and push normal vectors to normalsArray
    var BC = subtract(b, c);
    var CA = subtract(c, a);
    var AB = subtract(b, a);

    var c = cross(CA, AB);

	normalsArray.push(c);
	normalsArray.push(c);
	normalsArray.push(c);
    
    index += 3;
}

function size(v) {
    return Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

function resize_vec(v, l) {
    let s = size(v);
    return vec4(v[0] * l / s, v[1] * l / s, v[2] * l / s, 1.0);
}


function divideTriangle(a, b, c, count) {
    if (count === 0) 
    {
        triangle(a, b, c);
        return;
    }

    // you need to fix this one in your hw
	// triangle(a, b, c);
    let l = size(a);
    let AB = resize_vec(mix(a, b, 0.5), l);
    let BC = resize_vec(mix(b, c, 0.5), l);
    let CA = resize_vec(mix(c, a, 0.5), l);

    divideTriangle(a, AB, CA, count - 1);
    divideTriangle(b, AB, BC, count - 1);
    divideTriangle(c, BC, CA, count - 1);
    divideTriangle(AB, BC, CA, count - 1);


    // divideTriangle(a, AB, CA, count - 1);
}


function tetrahedron(a, b, c, d, n) {
    // this is the same idea as in Serpinski Gasket
    divideTriangle(a, b, c, n); // left
    divideTriangle(d, c, b, n); // front
    // debugger;
    divideTriangle(a, d, b, n); // also right?
    divideTriangle(a, c, d, n); // right
}

function main() {

    canvas = document.getElementById("gl-canvas");

    gl = canvas.getContext('webgl2');
    if (!gl) alert( "WebGL 2.0 isn't available");

    gl.viewport(0, 0, canvas.width, canvas.height);
    gl.clearColor(1.0, 1.0, 1.0, 1.0);

    gl.enable(gl.DEPTH_TEST);

    //
    //  Load shaders and initialize attribute buffers
    //
    var program = initShaders(gl, "vertex-shader", "fragment-shader");
    gl.useProgram(program);


    var ambientProduct = mult(lightAmbient, materialAmbient);
    var diffuseProduct = mult(lightDiffuse, materialDiffuse);
    var specularProduct = mult(lightSpecular, materialSpecular);

    tetrahedron(va, vb, vc, vd, numTimesToSubdivide);

    var nBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, nBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, flatten(normalsArray), gl.STATIC_DRAW);

    var normalLoc = gl.getAttribLocation(program, "aNormal");
    gl.vertexAttribPointer(normalLoc, 4, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(normalLoc);


    var vBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, flatten(positionsArray), gl.STATIC_DRAW);

    var positionLoc = gl.getAttribLocation( program, "aPosition");
    gl.vertexAttribPointer(positionLoc, 4, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(positionLoc);

    modelViewMatrixLoc = gl.getUniformLocation(program, "uModelViewMatrix");
    projectionMatrixLoc = gl.getUniformLocation(program, "uProjectionMatrix");
    nMatrixLoc = gl.getUniformLocation(program, "uNormalMatrix");

    document.getElementById("Button2").onclick = function(){theta += dr;};
    document.getElementById("Button3").onclick = function(){theta -= dr;};
    document.getElementById("Button4").onclick = function(){phi += dr;};
    document.getElementById("Button5").onclick = function(){phi -= dr;};

    document.getElementById("Button0").onclick = function(){
        numTimesToSubdivide++;
        index = 0;
        positionsArray = [];
        normalsArray = [];
        document.getElementById("triangle-count").innerText = 4 * Math.pow(4, numTimesToSubdivide) + ' triangles';
        main();
    };
    document.getElementById("Button1").onclick = function(){
        if(numTimesToSubdivide) numTimesToSubdivide--;
        index = 0;
        positionsArray = [];
        normalsArray = [];
        document.getElementById("triangle-count").innerText = 4 * Math.pow(4, numTimesToSubdivide) + ' triangles';
        main();
    };


    gl.uniform4fv( gl.getUniformLocation(program,
       "uAmbientProduct"), ambientProduct );
    gl.uniform4fv( gl.getUniformLocation(program,
       "uDiffuseProduct"), diffuseProduct );
    gl.uniform4fv( gl.getUniformLocation(program,
       "uSpecularProduct"), specularProduct );
    gl.uniform4fv( gl.getUniformLocation(program,
       "uLightPosition"), lightPosition );
    gl.uniform1f( gl.getUniformLocation(program,
       "uShininess"),materialShininess );

    render();
}

function render() {

    gl.clear( gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    eye = vec3(radius*Math.sin(theta)*Math.cos(phi),
        radius*Math.sin(theta)*Math.sin(phi), radius*Math.cos(theta));

    modelViewMatrix = lookAt(eye, at , up);
    projectionMatrix = ortho(left, right, bottom, top0, near, far);

    nMatrix = normalMatrix(modelViewMatrix, true);

    gl.uniformMatrix4fv(modelViewMatrixLoc, false, flatten(modelViewMatrix));
    gl.uniformMatrix4fv(projectionMatrixLoc, false, flatten(projectionMatrix));
    gl.uniformMatrix3fv(nMatrixLoc, false, flatten(nMatrix)  );

    for( var i=0; i<index; i+=3)
        gl.drawArrays( gl.TRIANGLES, i, 3 );

    const coord = 'Camera: (' + eye[0] + ', ' + eye[1] + ', ' + eye[2] + ')';
    document.getElementById("coords").innerText = coord;

    requestAnimationFrame(render);
}



main();
