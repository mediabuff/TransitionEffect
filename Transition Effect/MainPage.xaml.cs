using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media.Core;
using Windows.Media.MediaProperties;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using AdvancedMediaSource;
using Transition_Effect.Common;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=391641

namespace Transition_Effect
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private CAdvancedMediaSource advanced_media_source = null;
        private MediaStreamSource media_stream_source = null;
        private VideoStreamDescriptor videoDesc = null;
        private AudioStreamDescriptor audioDesc = null;

        private bool m_hasSetMediaSource = false;

        NavigationHelper navigationHelper;

        private const uint c_frameRateN = 30;
        private const uint c_frameRateD = 1;
        private const uint c_frameWidth = 640;
        private const uint c_frameHeight = 480;
        private const uint c_sampleRate = 44100;
        private const uint c_channelCount = 2;
        private const uint c_bitsPerSample = 16;

        public MainPage()
        {
            LoadVideos();
            LoadEffects();

            this.InitializeComponent();

            this.DataContext = this;
            this.NavigationCacheMode = NavigationCacheMode.Required;
            navigationHelper = new NavigationHelper(this);

            advanced_media_source = new CAdvancedMediaSource();
        }

        void InitializeMediaPlayer()
        {
            m_hasSetMediaSource = false;

            if (CurrentEffect == null || CurrentVideo == null)
                return;

            // Initialize Transition
            SecondVideo = VideoList.IndexOf(CurrentVideo) == VideoList.Count - 1 ? VideoList[0] : VideoList[VideoList.IndexOf(CurrentVideo) + 1];
            advanced_media_source.ResetTimeline();
            advanced_media_source.AddVideo(CurrentVideo);
            advanced_media_source.AddTransitionEffect(CurrentEffect.EffectType, 1);
            advanced_media_source.AddVideo(SecondVideo);

            // Initialize MediaStreamSource
            VideoEncodingProperties videoProperties = VideoEncodingProperties.CreateUncompressed(MediaEncodingSubtypes.Bgra8, c_frameWidth, c_frameHeight);
            videoDesc = new VideoStreamDescriptor(videoProperties);
            videoDesc.EncodingProperties.FrameRate.Numerator = c_frameRateN;
            videoDesc.EncodingProperties.FrameRate.Denominator = c_frameRateD;
            videoDesc.EncodingProperties.Bitrate = (uint)(c_frameRateN * c_frameRateD * c_frameWidth * c_frameHeight * 4);

            AudioEncodingProperties audioProperties = AudioEncodingProperties.CreatePcm(c_sampleRate, c_channelCount, c_bitsPerSample);
            audioDesc = new AudioStreamDescriptor(audioProperties);

            media_stream_source = new Windows.Media.Core.MediaStreamSource(videoDesc, audioDesc);

            TimeSpan spanBuffer = new TimeSpan(0, 0, 0, 0, 0);
            media_stream_source.BufferTime = spanBuffer;
            media_stream_source.Starting += MSS_Starting;
            media_stream_source.Closed += MSS_Closed;
            media_stream_source.SampleRequested += MSS_SampleRequested;

            Video.SetMediaStreamSource(media_stream_source);
            m_hasSetMediaSource = true;
        }

        void UninitializeMediaPlayer()
        {
            m_hasSetMediaSource = false;

            if (media_stream_source != null)
            {
                // NotifyError to shutdown MediaStreamSource
                media_stream_source.NotifyError(MediaStreamSourceErrorStatus.Other);
                Video.Stop();            
            }
        }

        private void Apply_Effect(object sender, RoutedEventArgs e)
        {
            UninitializeMediaPlayer();

            InitializeMediaPlayer();
        }

        void MSS_Starting(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceStartingEventArgs args)
        {
            if (!m_hasSetMediaSource || advanced_media_source == null)
                return;

            advanced_media_source.Initialize(media_stream_source, videoDesc, audioDesc);

            args.Request.SetActualStartPosition(new TimeSpan(0));
        }

        void MSS_Closed(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceClosedEventArgs args)
        {
            sender.Starting -= MSS_Starting;
            sender.Closed -= MSS_Closed;
            sender.SampleRequested -= MSS_SampleRequested;

            if (sender == media_stream_source)
            {
                media_stream_source = null;
            }
        }

        void MSS_SampleRequested(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceSampleRequestedEventArgs args)
        {
            if (!m_hasSetMediaSource || advanced_media_source == null)
                return;

            if (args.Request.StreamDescriptor is VideoStreamDescriptor)
            {
                advanced_media_source.GenerateVideoSample(args.Request);
            }
            else if (args.Request.StreamDescriptor is AudioStreamDescriptor)
            {
                advanced_media_source.GenerateAudioSample(args.Request);
            }
        }

        private void LoadEffects()
        {
            EffectList = new ObservableCollection<Effect>();
            EffectList.Add(new Effect() { Name = "Black",          EffectType = TransitionEffectType.TRANSITION_BLACK });
            EffectList.Add(new Effect() { Name = "White",          EffectType = TransitionEffectType.TRANSITION_WHITE });
            EffectList.Add(new Effect() { Name = "Black Fading",   EffectType = TransitionEffectType.TRANSITION_FADE_TO_BLACK });
            EffectList.Add(new Effect() { Name = "White Fading",   EffectType = TransitionEffectType.TRANSITION_FADE_TO_WHITE });
            EffectList.Add(new Effect() { Name = "Fading Transit", EffectType = TransitionEffectType.TRANSITION_FADE_OVERLAP });
            EffectList.Add(new Effect() { Name = "Right Left",     EffectType = TransitionEffectType.TRANSITION_RIGHT_TO_LEFT });
            EffectList.Add(new Effect() { Name = "Top Bottom",     EffectType = TransitionEffectType.TRANSITION_TOP_TO_BOTTOM });
            EffectList.Add(new Effect() { Name = "Bottom Top",     EffectType = TransitionEffectType.TRANSITION_BOTTOM_TO_TOP });
            EffectList.Add(new Effect() { Name = "ZoomIn",         EffectType = TransitionEffectType.TRANSITION_ROOM_IN });
            EffectList.Add(new Effect() { Name = "ZoomOut",        EffectType = TransitionEffectType.TRANSITION_ROOM_OUT });
        }

        private void LoadVideos()
        {
            VideoList = new ObservableCollection<string>();

            VideoList.Add(@"ms-appx:///Assets/Videos/big_buck_bunny_trailer_480p_high.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/1.wmv");
            VideoList.Add(@"ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4");

        }

        public string CurrentVideo { get; set; }
        public string SecondVideo { get; set; }
        public Effect CurrentEffect { get; set; }

        public ObservableCollection<string> VideoList { get; set; }
        public ObservableCollection<Effect> EffectList { get; set; }

        private void Playback_click(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(Playback));
        }


        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedFrom(e); 
            
            // As we are still playing we need to stop MSS from requesting for more samples otherwise we'll crash
            if (media_stream_source != null)
            {
                media_stream_source.NotifyError(MediaStreamSourceErrorStatus.Other);
                Video.Stop();
            }
        }

        private void Video_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            throw new NotSupportedException(e.ErrorMessage);
        }
    }
}

