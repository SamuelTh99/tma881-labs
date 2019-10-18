#!/usr/bin/julia

using LinearAlgebra: norm


parse_coordinate(s::String) :: Vector{Float32} = map(ss -> parse(Float32,ss), split(s))

counts = fill(zero(Int), 3465+1)
open("/home/hpc2019/a3_grading/test_data/cell_50") do file
  coordinates = map(parse_coordinate, eachline(file))

  for cx1 in 1:length(coordinates)
    for cx2 in (cx1+1):length(coordinates)
      dist = Int(trunc(100*norm(coordinates[cx1] .- coordinates[cx2]); digits = 0))
      counts[dist+1]+= 1
    end
  end
end

function digits(a::UInt,b::UInt,n::UInt) :: Vector{UInt}
  ds = UInt[]
  for i in 1:n
    push!(ds, a % b)
    a = div(a,b)
  end
  a != 0 && error("too few digits")
  return ds
end
digits(a::Int,b::Int,n::Int) = digits(UInt(a), UInt(b), UInt(n))

for dx in 1:length(counts)
  ds = digits(dx-1,10,4)
  println("$(ds[4])$(ds[3]).$(ds[2])$(ds[1]) $(counts[dx])")
end
