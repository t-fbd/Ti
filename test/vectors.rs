// Vectors - Fixed list where elements are the same data types

use std::mem;

pub fn run() {
    //    data type : length of Vector
    let mut numbers: Vec<i32> = vec![1, 2, 3, 4, 5];

    println!("{:?}", numbers);

    //add on
    numbers.push(6);
    numbers.push(7);

    //single value
    println!("Single Value: {}", numbers[0]);

    //get Vector length
    println!("Vector length: {}", numbers.len());

    //Vectors are stack allocated
    println!("Vector occupies {} bytes", mem::size_of_val(&numbers));

    //loop Vector
    for number in numbers.iter() {
        println!("Looped Values: {}", number);
    }

    // Get slice
    let mut num_slice: &[i32] = &numbers;
    println!("Slice: {:?}", num_slice);
    //first 2 numbers
    num_slice = &numbers[0..2];
    println!("Slice: {:?}", num_slice);
    //slice from 4 to end
    num_slice = &numbers[4..];
    println!("Slice: {:?}", num_slice);
    
    //loop & mutate
    for number in numbers.iter_mut() {
        *number += 1;
    }
    
    println!("numbers mutated: {:?}", numbers);
}
