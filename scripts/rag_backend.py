import os
import sys

# Optional: Try to import high-quality embedding libraries
try:
    from sentence_transformers import SentenceTransformer
    MODEL = SentenceTransformer('all-MiniLM-L6-v2')
    HAS_TRANSFORMERS = True
except ImportError:
    HAS_TRANSFORMERS = False

def get_embeddings(texts):
    """
    Generate embeddings for a list of strings.
    If sentence-transformers is available, use it (premium local RAG).
    Otherwise, fallback to a simple character-count-based mock for testing.
    """
    if HAS_TRANSFORMERS:
        embeddings = MODEL.encode(texts)
        return [e.tolist() for e in embeddings]
    
    # Fallback: simple mock embedding (384 dimensions to match MiniLM)
    results = []
    for text in texts:
        # Just a deterministic pseudo-random embedding based on text hash
        import hashlib
        h = hashlib.sha256(text.encode()).digest()
        # Create 384 floats
        embed = []
        for i in range(384):
            val = (h[i % 32] / 255.0) * (1.0 if i % 2 == 0 else -1.0)
            embed.append(val)
        results.append(embed)
    return results

if __name__ == "__main__":
    # Test
    test_texts = ["Hello world", "CodeHex is an AI agent"]
    print(get_embeddings(test_texts))
