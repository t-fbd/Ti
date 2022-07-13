
document.querySelector('#free1').addEventListener('submit', (e) => {
    const free1 = document.querySelector('#freeA1');
    if (free1.value == 'Lacy' || 'lacy') {
        free1.style.backgroundColor = 'green';
        free1.style.textAlign = 'center';
        free1.style.fontWeight = 'bold';
        free1.value = 'CORRECT!'
      }
    else {
        free1.style.backgroundColor = 'red';
        free1.style.textAlign = 'center';
        free1.style.fontWeight = 'bold';
        free1.value = 'INCORRECT!'
      }
    e.preventDefault()
});

//------------------------------------------------------------------free2

document.querySelector('#free2').addEventListener('submit', (e) => {
    const free2 = document.querySelector('#freeA2');
    if (free2.value == 'Brenna' || 'brenna') {
        free2.style.backgroundColor = 'green';
        free2.style.textAlign = 'center';
        free2.style.fontWeight = 'bold';
        free2.value = 'CORRECT!'
      }
    else {
        free2.style.backgroundColor = 'red';
        free2.style.textAlign = 'center';
        free2.style.fontWeight = 'bold';
        free2.value = 'INCORRECT!'
      }
    e.preventDefault()
});

//------------------------------------------------------------------mc1

const ma1 = document.querySelector('#mc1a1');
const ma2 = document.querySelector('#mc1a2');
const ma3 = document.querySelector('#mc1a3');
const ma4 = document.querySelector('#mc1a4');

ma1.addEventListener('click', (e) => {
    ma1.style.backgroundColor = 'green';
    ma1.style.textAlign = 'center';
    ma1.style.fontWeight = 'bold';
    ma1.innerText = 'CORRECT!';
    e.preventDefault()
});

ma2.addEventListener('click', (e) => {
    ma2.style.backgroundColor = 'red';
    ma2.style.textAlign = 'center';
    ma2.style.fontWeight = 'bold';
    ma2.innerText = 'INCORRECT!';
    e.preventDefault()
});

ma3.addEventListener('click', (e) => {
    ma3.style.backgroundColor = 'red';
    ma3.style.textAlign = 'center';
    ma3.style.fontWeight = 'bold';
    ma3.innerText = 'INCORRECT!';
    e.preventDefault()
});

ma4.addEventListener('click', (e) => {
    ma4.style.backgroundColor = 'red';
    ma4.style.textAlign = 'center';
    ma4.style.fontWeight = 'bold';
    ma4.innerText = 'INCORRECT!';
    e.preventDefault()
});

//-----------------------------------------------------------------------mc2

const m2a1 = document.querySelector('#mc2a1');
const m2a2 = document.querySelector('#mc2a2');
const m2a3 = document.querySelector('#mc2a3');
const m2a4 = document.querySelector('#mc2a4');

m2a1.addEventListener('click', (e) => {
    m2a1.style.backgroundColor = 'red';
    m2a1.style.textAlign = 'center';
    m2a1.style.fontWeight = 'bold';
    m2a1.innerText = 'INCORRECT!';
    e.preventDefault()
});

m2a2.addEventListener('click', (e) => {
    m2a2.style.backgroundColor = 'red';
    m2a2.style.textAlign = 'center';
    m2a2.style.fontWeight = 'bold';
    m2a2.innerText = 'INCORRECT!';
    e.preventDefault()
});

m2a3.addEventListener('click', (e) => {
    m2a3.style.backgroundColor = 'green';
    m2a3.style.textAlign = 'center';
    m2a3.style.fontWeight = 'bold';
    m2a3.innerText = 'CORRECT!';
    e.preventDefault()
});

m2a4.addEventListener('click', (e) => {
    m2a4.style.backgroundColor = 'red';
    m2a4.style.textAlign = 'center';
    m2a4.style.fontWeight = 'bold';
    m2a4.innerText = 'INCORRECT!';
    e.preventDefault()
});

