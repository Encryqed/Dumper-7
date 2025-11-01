// TODO: Benchmark offset finding performance
use criterion::{black_box, criterion_group, criterion_main, Criterion};

fn bench_placeholder(c: &mut Criterion) {
    c.bench_function("placeholder", |b| {
        b.iter(|| {
            black_box(42)
        });
    });
}

criterion_group!(benches, bench_placeholder);
criterion_main!(benches);
