struct Capsule{N, F} <: AbstractShape{N, F}
    a::Vec{N, F}
    b::Vec{N, F}
    radius::F
end

@inline Capsule(a::Vec{N, T1}, b::Vec{N, T2}, radius::Vec{N, T3}) where {N, T1, T2, T3} =
    Capsule{N, promote_type(T1, promote_type(T2, T3))}(a, b, radius)