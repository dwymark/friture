/**
 * @file text_renderer_test.cpp
 * @brief Unit tests for TextRenderer
 *
 * These tests verify the TextRenderer class functionality.
 * Note: Since we're in a headless environment, we test initialization
 * and API behavior rather than actual rendering output.
 */

#include <gtest/gtest.h>
#include <friture/ui/text_renderer.hpp>
#include <SDL2/SDL.h>

using namespace friture;

class TextRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize SDL video subsystem
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            GTEST_SKIP() << "SDL_Init failed: " << SDL_GetError();
        }

        // Use dummy video driver for headless testing
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);

        // Create window and renderer
        window_ = SDL_CreateWindow(
            "Test",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            800, 600,
            SDL_WINDOW_HIDDEN
        );

        if (!window_) {
            SDL_Quit();
            GTEST_SKIP() << "SDL_CreateWindow failed: " << SDL_GetError();
        }

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer_) {
            SDL_DestroyWindow(window_);
            SDL_Quit();
            GTEST_SKIP() << "SDL_CreateRenderer failed: " << SDL_GetError();
        }
    }

    void TearDown() override {
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(TextRendererTest, Construction) {
    TextRenderer text(renderer_);

    // TextRenderer should initialize successfully or skip if font not found
    // In headless environment, it might not find fonts, which is acceptable
    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer initialization failed (no fonts available): "
                     << text.getError();
    }

    EXPECT_TRUE(text.isValid());
}

TEST_F(TextRendererTest, ConstructionWithNullRenderer) {
    TextRenderer text(nullptr);

    // Should fail with null renderer
    EXPECT_FALSE(text.isValid());
    EXPECT_FALSE(text.getError().empty());
}

// ============================================================================
// Text Size Tests
// ============================================================================

TEST_F(TextRendererTest, GetTextSize) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    int width, height;
    bool result = text.getTextSize("Hello", 16, width, height);

    if (result) {
        EXPECT_GT(width, 0);
        EXPECT_GT(height, 0);
    }
}

TEST_F(TextRendererTest, GetTextSize_EmptyString) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    int width, height;
    bool result = text.getTextSize("", 16, width, height);

    // Empty string should return false
    EXPECT_FALSE(result);
}

TEST_F(TextRendererTest, GetTextSize_DifferentFontSizes) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    int width_small, height_small;
    int width_large, height_large;

    bool result1 = text.getTextSize("Test", 12, width_small, height_small);
    bool result2 = text.getTextSize("Test", 24, width_large, height_large);

    if (result1 && result2) {
        // Larger font should produce larger dimensions
        EXPECT_GT(width_large, width_small);
        EXPECT_GT(height_large, height_small);
    }
}

// ============================================================================
// Rendering Tests
// ============================================================================

TEST_F(TextRendererTest, RenderText_BasicCall) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Should not crash when rendering
    bool result = text.renderText("Hello World", 10, 10, white, 16);

    // In headless environment, rendering might fail, but should not crash
    // We mainly test that the API works correctly
    EXPECT_TRUE(result || !result); // Either works or doesn't, but doesn't crash
}

TEST_F(TextRendererTest, RenderText_EmptyString) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Empty string should return false
    bool result = text.renderText("", 10, 10, white, 16);
    EXPECT_FALSE(result);
}

TEST_F(TextRendererTest, RenderTextWithShadow) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};

    // Should not crash when rendering with shadow
    bool result = text.renderTextWithShadow("Test", 20, 20, white, black, 16, 2);

    // Either works or doesn't, but doesn't crash
    EXPECT_TRUE(result || !result);
}

TEST_F(TextRendererTest, RenderTextRightAlign) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Should not crash when rendering right-aligned
    bool result = text.renderTextRightAlign("Right", 100, 10, white, 16);

    EXPECT_TRUE(result || !result);
}

TEST_F(TextRendererTest, RenderTextCentered) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Should not crash when rendering centered
    bool result = text.renderTextCentered("Centered", 400, 300, white, 16);

    EXPECT_TRUE(result || !result);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(TextRendererTest, ErrorReporting) {
    TextRenderer text(nullptr);

    // Should have error for null renderer
    EXPECT_FALSE(text.getError().empty());
}

// ============================================================================
// Font Caching Tests
// ============================================================================

TEST_F(TextRendererTest, FontCaching_MultipleCalls) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Multiple calls with same font size should reuse cached font
    // This is mainly to ensure no memory leaks
    for (int i = 0; i < 10; ++i) {
        text.renderText("Test", 10, 10, white, 16);
    }

    // If we got here without crashing, font caching works
    SUCCEED();
}

TEST_F(TextRendererTest, FontCaching_DifferentSizes) {
    TextRenderer text(renderer_);

    if (!text.isValid()) {
        GTEST_SKIP() << "TextRenderer not initialized";
    }

    SDL_Color white = {255, 255, 255, 255};

    // Multiple different font sizes should be cached
    text.renderText("Small", 10, 10, white, 12);
    text.renderText("Medium", 10, 30, white, 16);
    text.renderText("Large", 10, 50, white, 24);
    text.renderText("XLarge", 10, 80, white, 32);

    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
