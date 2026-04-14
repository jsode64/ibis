/// A 2-dimensional vector.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Vec2<T> {
    pub x: T,

    pub y: T,
}

impl<T> Vec2<T> {
    /// Creates a vector with the given x and y factors.
    #[inline]
    #[must_use]
    pub const fn new(x: T, y: T) -> Self {
        Self { x, y }
    }
}
